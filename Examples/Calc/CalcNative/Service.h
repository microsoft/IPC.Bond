#include "generated/Calc_types.h"
#include <future>
#include <exception>
#include <sstream>
#include <iostream>


namespace Calc
{
    class Service
    {
    public:
        template <typename Callback>
        void operator()(std::future<Request> request, Callback&& callback)
        {
            try
            {
                callback(Process(request.get()));
            }
            catch (const std::exception& e)
            {
                std::cout << "Failed to send response: " << e.what() << std::endl;
            }
        }

    private:
        Response Process(const Request& request)
        {
            Response response;
            std::ostringstream text;
            text << request.X << ' ';

            switch (request.Op)
            {
            case Operation::Add:
                response.Z = request.X + request.Y;
                text << '+';
                break;

            case Operation::Subtract:
                response.Z = request.X - request.Y;
                text << '-';
                break;

            case Operation::Multiply:
                response.Z = request.X * request.Y;
                text << '*';
                break;

            case Operation::Divide:
                response.Z = request.X / request.Y;
                text << '/';
                break;
            }

            text << ' ' << request.Y << " = ";
            response.Text = text.str();

            return response;
        }
    };
}
