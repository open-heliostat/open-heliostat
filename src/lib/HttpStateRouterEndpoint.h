#ifndef HttpStateRouterEndpoint_h
#define HttpStateRouterEndpoint_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PsychicHttp.h>

#include <SecurityManager.h>
#include <StatefulService.h>

#ifndef HTTP_ENDPOINT_ORIGIN_ID
#define HTTP_ENDPOINT_ORIGIN_ID "http"
#endif
#ifndef HTTPS_ENDPOINT_ORIGIN_ID
#define HTTPS_ENDPOINT_ORIGIN_ID "https"
#endif

template <class T>
class HttpStateRouterEndpoint
{
public:
    HttpStateRouterEndpoint(JsonStateReader<T> stateReader,
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
        _securityManager(securityManager),
        _authenticationPredicate(authenticationPredicate),
        _wildcardPath(String(servicePath) + "/?*"),
        _pathLength(String(servicePath).length())
    {
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
                            String subPath = extractSubPath(path);
                            JsonObject nested = resolvePath(subPath, jsonObject);

                            JsonDocument requestBody;
                            if (deserializeJson(requestBody, request->body()) == DeserializationError::Ok)
                            {
                                nested.set(requestBody.as<JsonObject>());
                                ESP_LOGV(SVK_TAG, "HttpStateRouterEndpoint GET body: %s", requestBody.as<String>().c_str());
                            }

                            _statefulService->read(jsonObject, _stateReader);
                            response.getRoot() = nested;
                            return response.send();
                        },
                        _authenticationPredicate));
        ESP_LOGV(SVK_TAG, "Registered router GET endpoint: %s", _wildcardPath.c_str());

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
                            String subPath = extractSubPath(request->path());
                            JsonObject nested = resolvePath(subPath, payload);
                            nested.set(json.as<JsonObject>());

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
                            response.getRoot() = nested;

                            _statefulService->read(payload, _stateReader);
                            return response.send();
                        },
                        _authenticationPredicate));
        ESP_LOGV(SVK_TAG, "Registered router POST endpoint: %s", _wildcardPath.c_str());
    }

private:
    JsonObject resolvePath(const String &path, JsonObject root)
    {
        JsonObject current = root;
        if (path.length() == 0)
        {
            return current;
        }

        int start = 0;
        while (start < path.length())
        {
            int slash = path.indexOf('/', start);
            String segment;
            if (slash == -1)
            {
                segment = path.substring(start);
                start = path.length();
            }
            else
            {
                segment = path.substring(start, slash);
                start = slash + 1;
            }

            if (segment.length() == 0)
            {
                continue;
            }
            current = current[segment].to<JsonObject>();
        }
        return current;
    }

    String extractSubPath(const String &path) const
    {
        if (path.length() <= _pathLength)
        {
            return String();
        }
        return path.substring(_pathLength);
    }

    JsonStateReader<T> _stateReader;
    JsonStateUpdater<T> _stateUpdater;
    StatefulService<T> *_statefulService;
    SecurityManager *_securityManager;
    AuthenticationPredicate _authenticationPredicate;
    PsychicHttpServer *_server;
    String _wildcardPath;
    int _pathLength;
};

#endif
