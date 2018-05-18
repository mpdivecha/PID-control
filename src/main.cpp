#include <uWS/uWS.h>
#include <iostream>
#include <fstream>
#include "json.hpp"
#include "PID.h"
#include <math.h>

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s)
{
    auto found_null = s.find("null");
    auto b1 = s.find_first_of("[");
    auto b2 = s.find_last_of("]");
    if (found_null != std::string::npos)
    {
        return "";
    }
    else if (b1 != std::string::npos && b2 != std::string::npos)
    {
        return s.substr(b1, b2 - b1 + 1);
    }
    return "";
}

double max_speed_u = 50;
double max_speed_l = 48;
double throttleMean = 0.4;
double throttleMax = 0.7;

double handleMessage(uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode, PID &pid, double &throttle)
{
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
        auto s = hasData(std::string(data).substr(0, length));
        if (s != "")
        {
            auto j = json::parse(s);
            std::string event = j[0].get<std::string>();
            if (event == "telemetry")
            {
                // j[1] is the data JSON object
                double cte = std::stod(j[1]["cte"].get<std::string>());
                double speed = std::stod(j[1]["speed"].get<std::string>());
                double angle = std::stod(j[1]["steering_angle"].get<std::string>());
                double steer_value;
                /*
                 * TODO: Calcuate steering value here, remember the steering value is
                 * [-1, 1].
                 * NOTE: Feel free to play around with the throttle and speed. Maybe use
                 * another PID controller to control the speed!
                */
                pid.UpdateError(cte);
                steer_value = pid.TotalError();
                if (steer_value < -1)
                    steer_value = -1;
                else if (steer_value > 1)
                    steer_value = 1;

                if (throttle >= throttleMean && speed >= max_speed_u)
                    throttle -= 0.1;
                else if (throttle <= throttleMean && speed < max_speed_l)
                    throttle += 0.1;

                // DEBUG
                std::cout << "CTE: " << cte << " Steering Value: " << steer_value << std::endl;

                json msgJson;
                msgJson["steering_angle"] = steer_value;
                msgJson["throttle"] = throttle;
                auto msg = "42[\"steer\"," + msgJson.dump() + "]";
                //std::cout << msg << std::endl;
                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

                return cte;
            }
        }
        else
        {
            // Manual driving
            std::string msg = "42[\"manual\",{}]";
            ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
            return 0;
        }
    }
}

struct InfoPackage {
    int cnt = 0;
    double cte,speed,steering_angle;
    std::ofstream outfile;
};

int test(double sParams[], double tParams[], InfoPackage& info);
int twiddle();

int main()
{
    //return twiddle();

    InfoPackage pack;
    pack.outfile.open("temp.txt", std::ios::out);
    double sParams[3] = {0.15, 0.0, 3.31};        // {0.2, 0, 3.31};
    double tParams[3] = {0.1, 0, 1.0};
    std::vector<double> cte_history;

    int res = test(sParams, tParams, pack);

    //for (const auto &e : cte_history) outFile << e << "\n";
    return res;
}

/** Test the PID with some values
 * @param double[] sParams  The steering PID parameters
 * @param double[] tParams  The throttle PID parameters
 */
int test(double sParams[], double tParams[], InfoPackage& pack)
{
    uWS::Hub h;
    double throttle = throttleMean;

    PID pid;
    PID throttle_pid;
    // TODO: Initialize the pid variable.
    std::cout << "Initing PIDs\n";
    pid.Init(sParams[0], sParams[1], sParams[2]);
    throttle_pid.Init(tParams[0], tParams[1], tParams[2]);

    h.onMessage([&pid, &throttle, &throttle_pid, &pack](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
        // "42" at the start of the message means there's a websocket message event.
        // The 4 signifies a websocket message
        // The 2 signifies a websocket event
        //std::cout << std::string(data).substr(0, length) << std::endl;
        if (length && length > 2 && data[0] == '4' && data[1] == '2')
        {
            auto s = hasData(std::string(data).substr(0, length));
            if (s != "")
            {
                auto j = json::parse(s);
                std::string event = j[0].get<std::string>();
                //std::cout << "j: " << j << std::endl;
                if (event == "telemetry")
                {
                    // j[1] is the data JSON object
                    double cte = std::stod(j[1]["cte"].get<std::string>());
                    double speed = std::stod(j[1]["speed"].get<std::string>());
                    double angle = std::stod(j[1]["steering_angle"].get<std::string>());
                    double steer_value;
                    /*
                     * TODO: Calcuate steering value here, remember the steering value is
                     * [-1, 1].
                     * NOTE: Feel free to play around with the throttle and speed. Maybe use
                     * another PID controller to control the speed!
                    */
                    std::cout << "Updating pid\n";
                    pid.UpdateError(cte);
                    steer_value = pid.TotalError();
                    // Constrain the steering angle
                    if (steer_value < -1)
                        steer_value = -1;
                    else if (steer_value > 1)
                        steer_value = 1;

                    // Update and get throttle value. It is contrained to be around
                    //  the value of throttle we want
                    throttle_pid.UpdateError(cte);
                    throttle = throttleMean;// - throttle_pid.TotalError();

                    // Don't let throttle get beyond a certain maximum
                    if (throttle >= throttleMax)
                        throttle = throttleMax;

                    // DEBUG
                    pack.cnt++;
                    if (pack.cnt < 1250)
                    {
                        pack.cte = cte;
                        pack.speed = speed;
                        pack.steering_angle = angle;
                        pack.outfile << pack.cnt << "," << cte << "," << speed << "," << angle << "\n";
                    }
                    else if (pack.outfile.is_open() && pack.cnt >= 1250)
                    {
                        pack.outfile.close();
                    }
                    std::cout << "CTE: " << cte << " Steering Value: " << steer_value << " cnt: " << pack.cnt << std::endl;
                    //cte_history.push_back(cte);
                    //outfile << cte << "\n";
                    //outfile.flush();

                    json msgJson;
                    msgJson["steering_angle"] = steer_value;
                    msgJson["throttle"] = throttle;
                    auto msg = "42[\"steer\"," + msgJson.dump() + "]";
                    std::cout << msg << std::endl;
                    ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                }
            }
            else
            {
                // Manual driving
                std::string msg = "42[\"manual\",{}]";
                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
            }
        }
    });

    // We don't need this since we're not using HTTP but if it's removed the program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1)
        {
            res->end(s.data(), s.length());
        }
        else
        {
            // i guess this should be done more gracefully?
            res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        std::cout << "Connected!!!" << std::endl;
    });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
        ws.close();
        std::cout << "Disconnected" << std::endl;
    });

    int port = 4567;
    if (h.listen(port))
    {
        std::cout << "Listening to port " << port << std::endl;
    }
    else
    {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }
    h.run();
}

int twiddle()
{
    uWS::Hub h;

    PID pid;

    // Assuming the twiddle algo is like a finite state machine, then the
    //  following are the states of the FSM
    enum TwiddleGoto
    {
        INIT,
        LOOPCOVER,
        OUTERIF,
        OUTERELSE,
        CHECKSUM,
    };

    // Structure to store twiddle information to be passed between states and
    //  iterations
    struct TwiddleState
    {
        int curr_iter = 0;
        int curr_i = 0; // The current parameter
        TwiddleGoto stage;
        double p[3] = {1, 0, 3.31};
        double dp[3] = {1, 1, 1};
        double best_err = 9999;
    } state;

    state.stage = TwiddleGoto::INIT;

    double threshold = 0.01; // Threshold for sum_dp
    int iters = 1000;
    double best_p[] = {0, 0, 0};
    double throttle = throttleMean;
    //double dp[] = {1, 1, 1};

    // Init and run some iterations here. Compute the best_err
    double err = 0;
    pid.Init(state.p[0], state.p[1], state.p[2]);
    h.onMessage([&pid, &state, iters, &err, &best_p, threshold, &throttle](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
        std::cout << "curr_iter: " << state.curr_iter;
        std::cout << " p=[" << state.p[0] << ", " << state.p[1] << ", " << state.p[2] << "]";
        std::cout << " best_p=[" << best_p[0] << ", " << best_p[1] << ", " << best_p[2] << "]";
        std::cout << " dp=[" << state.dp[0] << ", " << state.dp[1] << ", " << state.dp[2] << "]";
        std::cout << " best_err: " << state.best_err;
        std::cout << " curr_err: " << err / abs(state.curr_iter - iters) << std::endl;

        if (state.curr_iter < 2 * iters)
        {
            // The run() function from the python code
            // This will only run when curr_iter < 2*iters. All other times the
            //  twiddle statess are hanled
            double cte = handleMessage(ws, data, length, opCode, pid, throttle);
            if (state.curr_iter > iters)
            {
                err += abs(cte); //pow(cte, 2);
            }
            state.curr_iter++;
        }
        else
        {
            // Twiddle state handling
            switch (state.stage)
            {
            // The very first run, before the iterations start
            case TwiddleGoto::INIT:
                state.best_err = err / iters;
                err = 0;
                state.stage = TwiddleGoto::CHECKSUM;
                break;
            // The while loop condition
            case TwiddleGoto::CHECKSUM:
            {
                std::cout << "Case:CHECKSUM" << std::endl;
                double sum_dp = state.dp[0] + state.dp[1] + state.dp[2];
                if (sum_dp > threshold)
                {
                    state.stage = TwiddleGoto::LOOPCOVER;
                    break;
                }
                else
                {
                    exit(0);
                }
            }
            // The for loop condition
            // We initialize the state and reset the simulator.
            // curr_iter is set to zero so we can get the data from the simulator
            case TwiddleGoto::LOOPCOVER:
            {
                std::cout << "Case:LOOPCOVER" << std::endl;
                state.p[state.curr_i] += state.dp[state.curr_i];
                pid.Init(state.p[0], state.p[1], state.p[2]);
                state.curr_iter = 0;
                state.stage = TwiddleGoto::OUTERIF;
                // Reset
                std::string msg = "42[\"reset\",{}]";
                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                state.curr_iter = 0;
                err = 0;
                std::cout << "Reset called" << std::endl;
                break;
            }
            // The outer if
            case TwiddleGoto::OUTERIF:
                std::cout << "Case:OUTERIF" << std::endl;
                err /= iters;
                if (err < state.best_err)
                {
                    state.best_err = err;
                    best_p[0] = state.p[0];
                    best_p[1] = state.p[1];
                    best_p[2] = state.p[2];
                    state.dp[state.curr_i] *= 1.2;

                    state.curr_i = (state.curr_i + 1);
                    if (state.curr_i < 3)
                        state.stage = TwiddleGoto::LOOPCOVER;
                    else
                    {
                        state.stage = TwiddleGoto::CHECKSUM;
                        state.curr_i = 0;
                    }
                    break;
                }
                else
                {
                    state.p[state.curr_i] -= 2 * state.dp[state.curr_i];
                    pid.Init(state.p[0], state.p[1], state.p[2]);
                    state.curr_iter = 0;
                    state.stage = TwiddleGoto::OUTERELSE;
                    // Reset
                    std::string msg = "42[\"reset\",{}]";
                    ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                    state.curr_iter = 0;
                    err = 0;
                    std::cout << "Reset called" << std::endl;
                    break;
                }
            case TwiddleGoto::OUTERELSE:
                std::cout << "Case:OUTERELSE" << std::endl;
                err /= iters;
                if (err < state.best_err)
                {
                    state.best_err = err;
                    best_p[0] = state.p[0];
                    best_p[1] = state.p[1];
                    best_p[2] = state.p[2];
                    state.dp[state.curr_i] *= 1.2;
                }
                else
                {
                    state.p[state.curr_i] += state.dp[state.curr_i];
                    state.dp[state.curr_i] *= 0.8;
                }
                state.curr_i = (state.curr_i + 1) % 3;
                state.stage = TwiddleGoto::LOOPCOVER;
                break;
            }
            // When handling twiddle state, send null values
            std::cout << "Next Iter" << std::endl;
            json msgJson;
            msgJson["steering_angle"] = 0;
            msgJson["throttle"] = 0;
            auto msg = "42[\"steer\"," + msgJson.dump() + "]";
            ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }
    });

    // We don't need this since we're not using HTTP but if it's removed the program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1)
        {
            res->end(s.data(), s.length());
        }
        else
        {
            // i guess this should be done more gracefully?
            res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        std::cout << "Connected!!!" << std::endl;
    });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
        ws.close();
        std::cout << "Disconnected" << std::endl;
    });

    int port = 4567;
    if (h.listen(port))
    {
        std::cout << "Listening to port " << port << std::endl;
    }
    else
    {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }

    h.run();
}
