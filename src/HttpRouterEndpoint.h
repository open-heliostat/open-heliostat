#ifndef HttpRouterEndpoint_h
#define HttpRouterEndpoint_h

#include <functional>

#include <PsychicHttp.h>

#include <SecurityManager.h>
#include <StatefulService.h>
#include <StatelessService.h>

#define HTTP_ENDPOINT_ORIGIN_ID "http"
#define HTTPS_ENDPOINT_ORIGIN_ID "https"

using namespace std::placeholders; // for `_1` etc

template <class T>
class HttpRouterEndpoint
{
protected:
    JsonStateReader<T> _stateReader;
    JsonStateUpdater<T> _stateUpdater;
    StatefulService<T> *_statefulService;
    SecurityManager *_securityManager;
    AuthenticationPredicate _authenticationPredicate;
    PsychicHttpServer *_server;
    const char *_servicePath;
    String wildcardPath;
    const int _pathLength;

public:
    HttpRouterEndpoint(JsonStateReader<T> stateReader,
                 JsonStateUpdater<T> stateUpdater,
                 StatefulService<T> *statefulService,
                 PsychicHttpServer *server,
                 const char *servicePath,
                 SecurityManager *securityManager,
                 AuthenticationPredicate authenticationPredicate = AuthenticationPredicates::IS_ADMIN) : _stateReader(stateReader),
                                                                                                         _stateUpdater(stateUpdater),
                                                                                                         _statefulService(statefulService),
                                                                                                         _server(server),
                                                                                                         _servicePath(servicePath),
                                                                                                         _securityManager(securityManager),
                                                                                                         _authenticationPredicate(authenticationPredicate),
                                                                                                         _pathLength(String(servicePath).length())
    {
        wildcardPath = String(servicePath) + "/?*";
    }

    JsonObject resolvePath(String path, JsonObject obj) {
        int index = 0;
        while ((index = path.indexOf('/')) != -1)
        {
            String segment = path.substring(0, index);
            ESP_LOGV("HTTP SUBPATH", "%s", segment.c_str());
            if (index > 0) obj = obj[segment].to<JsonObject>();
            path = path.substring(index+1);
        }
        if (path.length()>0) obj = obj[path].to<JsonObject>();
        return obj;
    }

    // register the web server on() endpoints
    void begin()
    {

// OPTIONS (for CORS preflight)
#ifdef ENABLE_CORS
        _server->on(wildcardPath.c_str(),
                    HTTP_OPTIONS,
                    _securityManager->wrapRequest(
                        [this](PsychicRequest *request)
                        {
                            return request->reply(200);
                        },
                        AuthenticationPredicates::IS_AUTHENTICATED));
#endif

        // GET
        _server->on(wildcardPath.c_str(),
                    HTTP_GET,
                    _securityManager->wrapRequest(
                        [this](PsychicRequest *request)
                        {
                            PsychicJsonResponse response = PsychicJsonResponse(request, false);
                            ESP_LOGV("HTTP GET", "Path : %s, Body: %s", request->path().c_str(), request->body().c_str());
                            JsonObject jsonObject = response.getRoot();
                            String path(request->path());
                            path = path.substring(_pathLength);
                            ESP_LOGV("HTTP GET", "Subpath %s", path.c_str());
                            JsonObject obj = resolvePath(path, jsonObject);
                            JsonDocument requestBody;
                            if (deserializeJson(requestBody, request->body()) == DeserializationError::Ok) {
                                obj.set(requestBody.as<JsonObject>());
                                ESP_LOGV("HTTP GET", "Json %s", requestBody.as<String>().c_str());   
                            }
                            _statefulService->read(jsonObject, _stateReader);
                            response.getRoot() = obj;
                            ESP_LOGV("HTTP GET", "Response %s", response.getRoot().as<String>().c_str());
                            return response.send();
                        },
                        _authenticationPredicate));
        ESP_LOGV("HttpRouterEndpoint", "Registered GET endpoint: %s", wildcardPath.c_str());

        // POST
        _server->on(wildcardPath.c_str(),
                    HTTP_POST,
                    _securityManager->wrapCallback(
                        [this](PsychicRequest *request, JsonVariant &json)
                        {
                            ESP_LOGV("HTTP POST", "Path : %s, Json : %s", request->path().c_str(), json.as<String>().c_str());

                            if (!json.is<JsonObject>())
                            {
                                return request->reply(400);
                            }

                            JsonDocument doc;
                            JsonObject jsonObject = doc.to<JsonObject>();
                            String path(request->path());
                            path = path.substring(_pathLength);
                            JsonObject obj = resolvePath(path, jsonObject);
                            obj.set(json.as<JsonObject>());

                            StateUpdateResult outcome = _statefulService->updateWithoutPropagation(jsonObject, _stateUpdater, request->path());

                            if (outcome == StateUpdateResult::ERROR)
                            {
                                return request->reply(400);
                            }
                            else if ((outcome == StateUpdateResult::CHANGED))
                            {
                                // persist the changes to the FS
                                _statefulService->callUpdateHandlers(HTTP_ENDPOINT_ORIGIN_ID);
                            }

                            PsychicJsonResponse response = PsychicJsonResponse(request, false);
                            response.getRoot() = obj;

                            _statefulService->read(jsonObject, _stateReader);

                            return response.send();
                        },
                        _authenticationPredicate));

        ESP_LOGV("HttpRouterEndpoint", "Registered POST endpoint: %s", wildcardPath.c_str());
    }
};

#endif
