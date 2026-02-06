#pragma once

#include <string>
#include <mutex>

class ApiClient {
public:
	ApiClient();
	~ApiClient();

	ApiClient(const ApiClient &) = delete;
	ApiClient &operator=(const ApiClient &) = delete;

	void configure(const std::string &url, const std::string &api_key);
	bool send_sr(int sr_value);
	bool is_configured() const;

private:
	std::string endpoint_url;
	std::string auth_key;
	mutable std::mutex config_mutex;
	bool curl_initialized;
};
