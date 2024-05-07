#pragma once

// syoch-robotics ライブラリから実数値をもらうコントローラーを持ってくる
#include <robotics/controller/float.hpp>
#include <robotics/controller/joystick.hpp>
#include <robotics/filter/angled_motor.hpp>
#include <robotics/controller/action.hpp>
namespace nhk2024b {
class RobotController {
 public:
  struct Config {
    //int right_id;
    //int left_id;
    //int arm_id;

    // コントローラーの ID リスト
   int stick_id;
   

  };

  // コントローラーから生えてるコントローラー Node リスト
  //controller::JoyStick right;
  //controller::JoyStick left;
  //controller::Action arm;
  controller:: JoyStick stick;
  


 
  RobotController(const Config& config):
   // right(config.right_id),
    //left(config.left_id),
    //arm(config.arm_id);
    stick(config.stick_id)
   

  

  {  }
};

class Robot {
  template<typename T>
  using Node = robotics::Node<T>;
  using joystick = robotics::JoyStick2D;


  // コントローラー定義
  RobotController ctrl;

 public:


  // 出力のやつ (モーターなど)
  Node<float> out_right;
  Node<float> out_left; 
   
   
  //Node<float> out_arm;
  // out_xxx の名前で定義してください
  Node<float> out_motor_0;
  Node<joystick> stick_;
using AngledMotor = robotics::filter::AngledMotor<float>;
  //がんばって？？？
  AngledMotor r1;
  AngledMotor l1;

  


  // 下記リンクのやつを使っても OK
  // https://github.com/nnctroboticsclub/syoch-robotics/tree/main/libs

  Robot(RobotController::Config &ctrl_config): ctrl(ctrl_config) {
    // 一番最初にしたい処理
  
    // なければ空で OK
  }

  void LinkController() {
    ctrl.stick.SetChangeCallback([this](robotics::types::JoyStick2D stick){
     
      out_right.SetValue(stick[1] -stick[0]);
      out_left.SetValue(stick[1] +stick[0]);

  
    });
    //ctrl.arm.Onfire();


    // コントローラー (ctrl オブジェクト) から色々生やす
    // この例ではコントローラー rotate の値をモーターの出力にリンクしている
    //ctrl.right.Link(out_right);
    //ctrl.left.Link(out_left);

   // ctrl.arm.Onfire([this](){
      //.AddAngle()
    //})
    
    // ctrl.rotate >> out_motor_0;
    // と書いても OK
  }
};
} // namespace nhk2024b