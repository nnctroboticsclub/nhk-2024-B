#include "app.hpp"

#include <mbed-robotics/ikakorobomas_node.hpp>
#include <robotics/node/node_inspector.hpp>

#include "bridge.hpp"

using robotics::utils::EMC;

template <typename T>
using Node = robotics::Node<T>;

Node<bool> dummy_emc;

class App::Impl {
  Config config_;

  //* Thread

  //* Network
  std::unique_ptr<Communication> com;
  IkakoRobomasNode roll;
  IkakoRobomasNode lock;

  //* EMC
  std::shared_ptr<EMC> emc = std::make_shared<EMC>();
  robotics::node::DigitalOut emc_out{
      std::make_shared<robotics::driver::Dout>(PC_1)};

  //* Components
  nhk2024b::Bridge bridge;
  //! some

  //* Threads
  std::shared_ptr<Thread> main_thread;

 public:
  Impl(App::Config &config) : com(std::make_unique<Communication>(config.com)) {
    auto keep_alive = emc->AddNode();
    this->com->can_.OnKeepAliveLost([keep_alive]() {
      printf("EMC(CAN) setted to %d\n", false);
      keep_alive->SetValue(false);
    });
    this->com->can_.OnKeepAliveRecovered([keep_alive]() {
      printf("EMC(CAN) setted to %d\n", true);
      keep_alive->SetValue(true);
    });

    auto controller_emc = emc->AddNode();
    dummy_emc >> *controller_emc;

    bridge.out_a >> roll.velocity;
    bridge.lock >> lock.velocity;

  }

  [[noreturn]] void MainThread() {
    robotics::Node<int> count;
    int i = 0;
    while (true) {
      printf("Count: %d\n", i);
      count.SetValue(i);

      i++;

      ThisThread::sleep_for(1s);
    }
  }

  [[noreturn]] void ReportThread() {
    while (true) {
      com->SendNonReactiveValues();
      com->Report();

      ThisThread::sleep_for(1ms);
    }
  }

  void Init() {
    printf("\x1b[1;32m-\x1b[m Init\n");

    printf("\x1b[1;32m|\x1b[m \x1b[32m-\x1b[m Main Thread\n");
    main_thread = std::make_shared<Thread>(osPriorityNormal, 1024 * 4);
    main_thread->start(callback(this, &App::Impl::MainThread));

    printf("\x1b[1;32m|\x1b[m \x1b[32m-\x1b[m EMC\n");
    emc->output.Link(emc_out);
    emc->Init();

    printf("\x1b[1;32m|\x1b[m \x1b[32m-\x1b[m COM\n");
    com->Init();
    if (config_.can1_debug) com->AddCAN1Debug();

    printf("\x1b[1;32m|\x1b[m \x1b[32m-\x1b[m Node Inspector\n");
    robotics::node::NodeInspector::RegisterCAN(
        std::shared_ptr<robotics::network::DistributedCAN>(&com->can_));

    printf("\x1b[1;32m+\x1b[m   \x1b[33m+\x1b[m\n");

    com->SetStatus(robotics::network::can_module::Status::Statuses::kReady);
  }
};

App::App(Config &config) : impl(new Impl(config)) {}

void App::Init() { this->impl->Init(); }