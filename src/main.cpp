#include <uWS/uWS.h>
#include <iostream>
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
std::string hasData(std::string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != std::string::npos) {
    return "";
  }
  else if (b1 != std::string::npos && b2 != std::string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}


double handleMessage(uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode, PID& pid, double& throttle)
{
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
      auto s = hasData(std::string(data).substr(0, length));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
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

          if (throttle >= 0.3 && speed >= 30)
            throttle -= 0.1;
          else if (throttle <= 0.3 && speed < 36)
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
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        return 0;
      }
    }
}

int test();
int twiddle();

double max_speed_u = 50;
double max_speed_l = 48;
double throttleMean = 0.4;
double throttleMax = 0.7;

int main()
{
    return test();
}

int test()
{
  uWS::Hub h;
  double throttle = throttleMean;

  PID pid;
  PID throttle_pid;
  // TODO: Initialize the pid variable.
  //pid.Init(12.1619, 0.000269722, 0.999783);
  pid.Init(0.2, 0, 3.31);
  throttle_pid.Init(0.1, 0, 1.0);

  h.onMessage([&pid, &throttle, &throttle_pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
      auto s = hasData(std::string(data).substr(0, length));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
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

          throttle_pid.UpdateError(cte);
          throttle = throttleMean - throttle_pid.TotalError();

          if (throttle >= throttleMax)
            throttle = throttleMax;

        //   if (throttle >= throttleMean && speed >= max_speed_u)
        //     throttle -= 0.1;
        //   else if (throttle <= throttleMean && speed < max_speed_l)
        //     throttle += 0.1;
          
          // DEBUG
          std::cout << "CTE: " << cte << " Steering Value: " << steer_value << std::endl;

          json msgJson;
          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = throttle;
          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          std::cout << msg << std::endl;
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }
      } else {
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
  // TODO: Initialize the pid variable.
  //pid.Init(-0.5, 0.0, -1.00);

  enum TwiddleGoto {
      INIT,
      LOOPCOVER,
      OUTERIF,
      OUTERELSE,
      CHECKSUM,
      INNERIF,
      INNERELSE,
  };

  struct TwiddleState {
    int curr_iter = 0;
    int curr_i = 0;
    TwiddleGoto stage;
    //double p[3] = {12.1619, 0.000269722, 0.999783};   
    //double p[3] = {0, 0, 0};
    double p[3] = {1, 0, 3.31};
    double dp[3] = {1, 1, 1};
    double best_err = 9999;
  } state;

  state.stage = TwiddleGoto::INIT;

  double threshold = 0.01;
  int iters = 1000;
  double best_p[] = {0, 0, 0};
  double throttle = 0.4;
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
    std::cout << " curr_err: " << err/abs(state.curr_iter - iters) << std::endl;

    if (state.curr_iter < 2*iters) {
        // Implement the run() of the python code here
        double cte = handleMessage(ws, data, length, opCode, pid, throttle);
        if (state.curr_iter > iters)
        {
            err += abs(cte);//pow(cte, 2);
        }
        state.curr_iter++;
        //std::cout << "Message handling 1, curr_iter: " << state.curr_iter << std::endl;
    }
    else
    {
        // Implement the twiddle algorithm here
        switch(state.stage) {
            case TwiddleGoto::INIT:
                state.best_err = err / iters;
                err = 0;
                state.stage = TwiddleGoto::CHECKSUM;
                break;
            case TwiddleGoto::CHECKSUM:
            {
                std::cout << "Case:CHECKSUM" << std::endl;
                double sum_dp = state.dp[0] + state.dp[1] + state.dp[2];
                if (sum_dp > threshold)
                {
                    state.stage = TwiddleGoto::LOOPCOVER;
                    break;
                }
                else {
                    exit(0);
                }
            }
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
            case TwiddleGoto::OUTERIF:
                std::cout << "Case:OUTERIF" << std::endl;
                err /= iters;
                if (err < state.best_err)
                {
                    state.best_err = err;
                    best_p[0] = state.p[0];best_p[1] = state.p[1];best_p[2] = state.p[2];
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
                    best_p[0] = state.p[0];best_p[1] = state.p[1];best_p[2] = state.p[2];
                    state.dp[state.curr_i] *= 1.2;
                }
                else
                {
                    state.p[state.curr_i] += state.dp[state.curr_i];
                    state.dp[state.curr_i] *= 0.8;
                }
                state.curr_i = (state.curr_i + 1)%3;
                state.stage = TwiddleGoto::LOOPCOVER;
                break;
        }
        std::cout << "Next Iter" << std::endl;
        json msgJson;
        msgJson["steering_angle"] = 0;
        msgJson["throttle"] = 0;
        auto msg = "42[\"steer\"," + msgJson.dump() + "]";
        //std::cout << msg << std::endl;
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

        // Compare the error here. Outer if func()
        // Outer else. Func()
            // Inner If. Func()
            // Inner else. Func()


        // Then reset
        // std::string msg = "42[\"reset\",{}]";
        // ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        // state.curr_iter = 0;
        // std::cout << "Reset called" << std::endl;

        // Call the loop cover here
        // pid.Init(state.p[0], state.p[1], state.p[2]);
    }
  });
  
//   double sum_dp = dp[0] + dp[1] + dp[2];
//   while (sum_dp > threshold)
//   {
//     for(int i = 0; i < 3; i++)
//     {
//         p[i] += dp[i];

//         PID pid;
//         pid.Init(p[0], p[1], p[2]);

//         // onMessage request here and get the err
//         double err;
//         h.onMessage([&pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
//             //handleMessage(ws, data, length, opCode, pid);
//             std::cout << "Message handling 2" << std::endl;
//         });

//         if (err < best_err)
//         {
//             best_err = err;
//             dp[i] *= 1.1;
//         }
//         else
//         {
//             p[i] -= 2*dp[i];

//             PID pid;
//             pid.Init(p[0], p[1], p[2]);

//             // onMessage request here and get the err
//             err = 9999;
//             h.onMessage([&pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
//                 //handleMessage(ws, data, length, opCode, pid);
//                 std::cout << "Message handling 3" << std::endl;
//             });

//             if (err < best_err)
//             {
//                 best_err = err;
//                 dp[i] *= 1.1;
//             }
//             else
//             {
//                 p[i] += dp[i];
//                 dp[i] *= 0.9;
//             }
//         }


//     }
//   }

//   h.onMessage([&pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
//     handleMessage(ws, data, length, opCode, pid);
//   });
    
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
