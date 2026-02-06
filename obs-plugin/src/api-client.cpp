#include "api-client.h"
#include "plugin-support.h"

#include <curl/curl.h>
#include <ctime>
#include <string>
#include <sstream>

// Discard response body
static size_t write_discard(void *, size_t size, size_t nmemb, void *)
{
	return size * nmemb;
}

ApiClient::ApiClient() : curl_initialized(false)
{
	if (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK)
		curl_initialized = true;
	else
		sr_log_error("curl_global_init failed");
}

ApiClient::~ApiClient()
{
	if (curl_initialized)
		curl_global_cleanup();
}

void ApiClient::configure(const std::string &url, const std::string &api_key)
{
	std::lock_guard<std::mutex> lock(config_mutex);
	endpoint_url = url;
	auth_key = api_key;

	if (!url.empty())
		sr_log_info("API client configured: %s", url.c_str());
}

bool ApiClient::is_configured() const
{
	std::lock_guard<std::mutex> lock(config_mutex);
	return !endpoint_url.empty() && !auth_key.empty();
}

bool ApiClient::send_sr(int sr_value)
{
	std::string url;
	std::string key;

	{
		std::lock_guard<std::mutex> lock(config_mutex);
		url = endpoint_url;
		key = auth_key;
	}

	if (url.empty() || key.empty())
		return false;

	if (!curl_initialized)
		return false;

	CURL *curl = curl_easy_init();
	if (!curl) {
		sr_log_warn("curl_easy_init failed");
		return false;
	}

	// Build JSON payload
	std::time_t now = std::time(nullptr);
	std::ostringstream json;
	json << "{\"sr\":" << sr_value << ",\"timestamp\":" << now << "}";
	std::string body = json.str();

	// Build auth header
	std::string auth_header = "Authorization: Bearer " + key;

	struct curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, auth_header.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_discard);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "obs-sr-tracker/1.0");

	CURLcode res = curl_easy_perform(curl);

	bool success = false;
	if (res == CURLE_OK) {
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code >= 200 && http_code < 300) {
			sr_log_info("SR %d posted to API (HTTP %ld)", sr_value,
				    http_code);
			success = true;
		} else {
			sr_log_warn("API returned HTTP %ld for SR %d",
				    http_code, sr_value);
		}
	} else {
		sr_log_warn("API request failed: %s",
			    curl_easy_strerror(res));
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return success;
}
