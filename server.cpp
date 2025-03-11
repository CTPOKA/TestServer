#include "crow.h"
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <json/json.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <regex>
#include <string>

int image_id = 0;

const std::string GITHUB_TOKEN = std::getenv("GITHUB_TOKEN") ? std::getenv("GITHUB_TOKEN") : "";
const std::string REPO_OWNER = "CTPOKA";
const std::string REPO_NAME = "CTPOKA";
const std::string FILE_PATH = "README.md";
const std::string BRANCH = "main";

std::string base64_encode(const std::string &input)
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), input.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string encoded(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return encoded;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

std::string base64_decode(const std::string &input)
{
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T[base64_chars[i]] = i;

    std::string decoded;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1)
            continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

size_t write_callback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    output->append((char *) contents, size * nmemb);
    return size * nmemb;
}

std::pair<std::string, std::string> get_readme_content()
{
    std::string url = "https://api.github.com/repos/" + REPO_OWNER + "/" + REPO_NAME + "/contents/"
                      + FILE_PATH;
    CURL *curl = curl_easy_init();
    std::string response_data;

    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
        headers = curl_slist_append(headers, ("Authorization: token " + GITHUB_TOKEN).c_str());
        headers = curl_slist_append(headers, "User-Agent: MyGitHubBot");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    std::cout << "GitHub API response: " << response_data << std::endl;

    Json::Value jsonData;
    Json::CharReaderBuilder reader;
    std::istringstream s(response_data);
    std::string errs;
    Json::parseFromStream(reader, s, &jsonData, &errs);

    std::string content = jsonData["content"].asString();
    std::string sha = jsonData["sha"].asString();

    content = base64_decode(content);

    std::cout << "Decoded README content: " << content << std::endl;

    return {content, sha};
}

std::string update_readme_content(const std::string &content, const std::string &new_text)
{
    std::regex pattern(
        R"((<img\b[^>]*class="[^"]*dynamic[^"]*"[^>]*src=")(https:\/\/[a-zA-Z0-9\/.]+)("[^>]*>))");
    return std::regex_replace(content, pattern, "$1" + new_text + "$3");
}

void update_readme(const std::string &new_content)
{
    std::string url = "https://api.github.com/repos/" + REPO_OWNER + "/" + REPO_NAME + "/contents/"
                      + FILE_PATH;
    auto [current_content, sha] = get_readme_content();

    std::cout << "Current SHA: " << sha << std::endl;

    std::string encoded_content = base64_encode(update_readme_content(current_content, new_content));

    Json::Value jsonData;
    jsonData["message"] = "Update README.md";
    jsonData["content"] = encoded_content;
    jsonData["sha"] = sha;
    jsonData["branch"] = BRANCH;

    Json::StreamWriterBuilder writer;
    std::string payload = Json::writeString(writer, jsonData);

    std::cout << "Payload to GitHub: " << payload << std::endl;

    CURL *curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
        headers = curl_slist_append(headers, ("Authorization: token " + GITHUB_TOKEN).c_str());
        headers = curl_slist_append(headers, "User-Agent: MyGitHubBot");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        std::cout << "GitHub API update response: " << response << std::endl;
    }
}

std::string url_encode(const std::string &value)
{
    CURL *curl = curl_easy_init();
    char *encoded = curl_easy_escape(curl, value.c_str(), value.length());
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

std::vector<std::string> get_images(const std::string &query)
{
    std::string search_url = "https://yandex.ru/images/search?text=" + url_encode(query);
    std::string response_data;

    CURL *curl = curl_easy_init();
    if (!curl)
        return {};

    curl_easy_setopt(curl, CURLOPT_URL, search_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    std::vector<std::string> urls;
    std::regex img_regex(R"(&quot;(https:\/\/[a-zA-Z0-9\/.]+\.jpg))");

    auto words_begin = std::sregex_iterator(response_data.begin(), response_data.end(), img_regex);
    auto words_end = std::sregex_iterator();

    for (auto it = words_begin; it != words_end; ++it)
        urls.push_back((*it)[1]);

    return urls;
}

int main()
{
    crow::SimpleApp app;
    srand(time(0));
    auto images = get_images("фото смешных котов");

    CROW_ROUTE(app, "/next")([&images] {
        if (images.size() > 1) {
            image_id = (image_id + 7) % images.size();
            update_readme(images[image_id]);
        }
        crow::response res(307);
        std::string redirect_url = "https://github.com/" + REPO_OWNER;
        res.set_header("Location", redirect_url);
        return res;
    });

    app.port(1234).multithreaded().run();
}
