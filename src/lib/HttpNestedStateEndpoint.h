#ifndef HttpNestedStateEndpoint_h
#define HttpNestedStateEndpoint_h

#include <functional>

#include <PsychicHttp.h>

#include <SecurityManager.h>
#include <StatefulService.h>

#define HTTP_ENDPOINT_ORIGIN_ID "http"
#define HTTPS_ENDPOINT_ORIGIN_ID "https"

using namespace std::placeholders; // for `_1` etc

template <class T>
class HttpNestedStateEndpoint
{
protected:
    JsonStateReader<T> _stateReader;
    JsonStateUpdater<T> _stateUpdater;
    StatefulService<T> *_statefulService;
    SecurityManager *_securityManager;
    AuthenticationPredicate _authenticationPredicate;
    PsychicHttpServer *_server;
    const char *_servicePath;
    String _wildcardPath;
    int _pathLength;

public:
    HttpNestedStateEndpoint(JsonStateReader<T> stateReader,
                            JsonStateUpdater<T> stateUpdater,
                            StatefulService<T> *statefulService,
                            PsychicHttpServer *server,
                            const char *servicePath,
                            SecurityManager *securityManager,
                            AuthenticationPredicate authenticationPredicate = AuthenticationPredicates::IS_ADMIN) :
        _stateReader(stateReader),
        _stateUpdater(stateUpdater),
        _statefulService(statefulService),
        _server(server),
        _servicePath(servicePath),
        _securityManager(securityManager),
        _authenticationPredicate(authenticationPredicate)
    {
        _wildcardPath = String(servicePath) + "/?*";
        _pathLength = String(servicePath).length();
    }

    JsonObject resolvePath(String path, JsonObject obj)
    {
        while (true)
        {
            int index = path.indexOf('/');
            if (index == -1)
            {
                if (path.length() > 0)
                {
                    obj = obj[path].to<JsonObject>();
                }
                break;
            }

            String segment = path.substring(0, index);
            if (segment.length() > 0)
            {
                obj = obj[segment].to<JsonObject>();
            }
            path = path.substring(index + 1);
        }
        return obj;
    }

    void begin()
    {
#ifdef ENABLE_CORS
        _server->on(_wildcardPath.c_str(),
                    HTTP_OPTIONS,
                    _securityManager->wrapRequest(
                        [this](PsychicRequest *request)
                        {
                            return request->reply(200);
                        },
                        AuthenticationPredicates::IS_AUTHENTICATED));
#endif

        _server->on(_wildcardPath.c_str(),
                    HTTP_GET,
                    _securityManager->wrapRequest(
                        [this](PsychicRequest *request)
                        {
                            PsychicJsonResponse response(request, false);
                            JsonObject jsonObject = response.getRoot();

                            String path(request->path());
                            path = path.substring(_pathLength);
                            JsonObject subObject = resolvePath(path, jsonObject);

                            JsonDocument requestBody;
                            if (deserializeJson(requestBody, request->body()) == DeserializationError::Ok)
                            {
                                subObject.set(requestBody.as<JsonObject>());
                                ESP_LOGV(SVK_TAG, "Nested GET request extended state: %s", requestBody.as<String>().c_str());
                            }

                            _statefulService->read(jsonObject, _stateReader);
                            response.getRoot() = subObject;
                            return response.send();
                        },
                        _authenticationPredicate));
        ESP_LOGV(SVK_TAG, "Registered nested GET endpoint: %s", _wildcardPath.c_str());

        _server->on(_wildcardPath.c_str(),
                    HTTP_POST,
                    _securityManager->wrapCallback(
                        [this](PsychicRequest *request, JsonVariant &json)
                        {
                            if (!json.is<JsonObject>())
                            {
                                return request->reply(400);
                            }

                            JsonDocument doc;
                            JsonObject payload = doc.to<JsonObject>();
                            String path(request->path());
                            path = path.substring(_pathLength);
                            JsonObject subObject = resolvePath(path, payload);
                            subObject.set(json.as<JsonObject>());

                            StateUpdateResult outcome = _statefulService->updateWithoutPropagation(payload, _stateUpdater, request->path());

                            if (outcome == StateUpdateResult::ERROR)
                            {
                                return request->reply(400);
                            }
                            else if (outcome == StateUpdateResult::CHANGED)
                            {
                                _statefulService->callUpdateHandlers(HTTP_ENDPOINT_ORIGIN_ID);
                            }

                            PsychicJsonResponse response(request, false);
                            response.getRoot() = subObject;

                            _statefulService->read(payload, _stateReader);
                            return response.send();
                        },
                        _authenticationPredicate));
        ESP_LOGV(SVK_TAG, "Registered nested POST endpoint: %s", _wildcardPath.c_str());
    }
};

#endif
