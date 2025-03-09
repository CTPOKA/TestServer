#include "crow.h"
#include <cstdlib>
#include <ctime>
#include <fstream>

int image_id = 0;
int image_count = 4;

void next_image()
{
    image_id = (image_id + 1) % image_count;
}

int main()
{
    crow::SimpleApp app;
    srand(time(0));

    CROW_ROUTE(app, "/image.jpg")([] {
        std::stringstream ss;
        ss << PROJECT_ROOT << "/resources/image" << image_id << ".jpg";
        std::ifstream file(ss.str(), std::ios::binary);

        std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        crow::response res;
        res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, proxy-revalidate");
        res.set_header("Content-Type", "image/jpg");
        res.body = std::string(buffer.begin(), buffer.end());
        return res;
    });

    CROW_ROUTE(app, "/next-image")([] {
        next_image();
        crow::response res(307);
        res.set_header("Location", "https://github.com/CTPOKA");
        return res;
    });

    app.port(1234).multithreaded().run();
}
