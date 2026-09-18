#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// =====================================================
// WIFI
// =====================================================

const char* ssid = "My-RC-Car";
const char* password = "password123";

WebServer server(80);
Servo steeringServo;


// =====================================================
// PINS
// =====================================================

const int IN1 = D2;
const int IN2 = D1;
const int SERVO_PIN = D10;


// =====================================================
// MOTOR SETTINGS
// =====================================================

const int MIN_DRIVE = 80;

const int MAX_FORWARD = 128;
const int MAX_REVERSE = 90;

// How fast the car accelerates
const float ACCELERATION = 90.0;

// Reverse acceleration
const float REVERSE_ACCELERATION = 120.0;

// Braking when changing direction
const float BRAKE_DECELERATION = 300.0;

// Natural coasting after releasing throttle
const float COAST_DECELERATION = 70.0;

// Motor deadzone
const int MOTOR_DEADZONE = 15;

float currentSpeed = 0;
int targetSpeed = 0;


// =====================================================
// STEERING
// =====================================================

const int CENTER_ANGLE = 108;
const int LEFT_ANGLE = 63;
const int RIGHT_ANGLE = 153;

const int SERVO_MIN_LIMIT = 30;
const int SERVO_MAX_LIMIT = 150;

// Keyboard steering speed
const float STEER_SPEED = 180.0;

// How quickly steering returns to center
const float STEER_RETURN_SPEED = 220.0;

float currentSteer = CENTER_ANGLE;

int steeringInput = 0;

int trimOffset = 0;

// True when phone slider is controlling steering
bool phoneSteering = false;


// =====================================================
// HTML
// =====================================================

String htmlPage = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;

  width: 100vw;
  height: 100vh;

  background: #0f0f12;
  color: white;

  font-family: Arial, sans-serif;

  display: flex;
  justify-content: center;
  align-items: center;

  overflow: hidden;
  touch-action: none;
}

.container {

  width: 100vw;
  height: 100vh;

  display: flex;
  justify-content: center;
  align-items: center;

  gap: 20px;
}

input[type=range] {

  appearance: none;
  -webkit-appearance: none;

  background: #1a1a20;

  outline: none;

  border-radius: 60px;

  border: 4px solid #00ff88;
}

input[type=range]::-webkit-slider-thumb {

  appearance: none;
  -webkit-appearance: none;

  width: 80px;
  height: 80px;

  border-radius: 50%;

  background: #00ff88;

  box-shadow: 0 0 15px #00ff88;
}

.speed {

  width: 75vh;
  height: 110px;

  transform: rotate(-90deg);
}

.steer {

  width: 40vw;
  height: 110px;
}

.trim {

  width: 60vh;
  height: 30px;

  transform: rotate(-90deg);

  border-color: #ff0055 !important;
}

.trim::-webkit-slider-thumb {

  width: 45px;
  height: 45px;

  background: #ff0055;

  box-shadow: 0 0 10px #ff0055;
}

#info {

  position: absolute;

  top: 12px;
  left: 0;

  width: 100%;

  text-align: center;

  font-family: monospace;

  font-size: 18px;
}

#help {

  position: absolute;

  bottom: 10px;

  width: 100%;

  text-align: center;

  color: #555;

  font-size: 13px;
}

</style>

</head>


<body>


<div id="info">

  RC CAR &nbsp; | &nbsp;

  TRIM:
  <span id="trimValue">0</span>

</div>


<div class="container">


<!-- ================= SPEED ================= -->

<input
  id="speedSlider"
  class="speed"

  type="range"

  min="-90"
  max="128"

  value="0"

  oninput="sendSpeed(this.value)"

  onpointerup="releaseSpeed()"
>


<!-- ================= TRIM ================= -->

<input
  id="trimSlider"
  class="trim"

  type="range"

  min="-25"
  max="25"

  value="0"

  oninput="sendTrim(this.value)"
>


<!-- ================= STEERING ================= -->

<input
  id="steerSlider"
  class="steer"

  type="range"

  min="63"
  max="153"

  value="108"

  oninput="sendSteer(this.value)"

  onpointerup="releaseSteer()"
>


</div>


<div id="help">

PC: W/S = throttle &nbsp;&nbsp;
A/D = steering &nbsp;&nbsp;
Q/E = trim

</div>


<script>


// =====================================================
// CONSTANTS
// =====================================================

const CENTER_ANGLE = 108;


// =====================================================
// PHONE SPEED
// =====================================================

function sendSpeed(value) {

  fetch("/setSpeed?val=" + value)
    .catch(() => {});

}


function releaseSpeed() {

  const slider =
    document.getElementById("speedSlider");

  slider.value = 0;

  sendSpeed(0);

}


// =====================================================
// PHONE STEERING
// =====================================================

function sendSteer(value) {

  fetch("/setSteer?val=" + value)
    .catch(() => {});

}


function releaseSteer() {

  const slider =
    document.getElementById("steerSlider");

  slider.value = CENTER_ANGLE;

  sendSteer(CENTER_ANGLE);

}


// =====================================================
// TRIM
// =====================================================

function sendTrim(value) {

  document.getElementById("trimValue").innerText = value;

  fetch("/setTrim?val=" + value)
    .catch(() => {});

}


// =====================================================
// KEYBOARD STATE
// =====================================================

let wPressed = false;
let sPressed = false;

let aPressed = false;
let dPressed = false;


// =====================================================
// KEYBOARD DOWN
// =====================================================

document.addEventListener("keydown", function(e) {

  if(e.repeat)
    return;


  // -------------------------
  // W
  // -------------------------

  if(e.code === "KeyW") {

    wPressed = true;

    phoneSteering = false;

    sendSpeed(128);

  }


  // -------------------------
  // S
  // -------------------------

  if(e.code === "KeyS") {

    sPressed = true;

    phoneSteering = false;

    sendSpeed(-90);

  }


  // -------------------------
  // A
  // -------------------------

  if(e.code === "KeyA") {

    aPressed = true;

    phoneSteering = false;

    fetch("/steerKey?dir=-1");

  }


  // -------------------------
  // D
  // -------------------------

  if(e.code === "KeyD") {

    dPressed = true;

    phoneSteering = false;

    fetch("/steerKey?dir=1");

  }


  // -------------------------
  // Q TRIM
  // -------------------------

  if(e.code === "KeyQ") {

    let slider =
      document.getElementById("trimSlider");

    let value =
      parseInt(slider.value) - 1;

    value =
      Math.max(-25, value);

    slider.value = value;

    sendTrim(value);

  }


  // -------------------------
  // E TRIM
  // -------------------------

  if(e.code === "KeyE") {

    let slider =
      document.getElementById("trimSlider");

    let value =
      parseInt(slider.value) + 1;

    value =
      Math.min(25, value);

    slider.value = value;

    sendTrim(value);

  }

});


// =====================================================
// KEYBOARD UP
// =====================================================

document.addEventListener("keyup", function(e) {


  // -------------------------
  // W
  // -------------------------

  if(e.code === "KeyW") {

    wPressed = false;

    if(!sPressed)
      sendSpeed(0);

  }


  // -------------------------
  // S
  // -------------------------

  if(e.code === "KeyS") {

    sPressed = false;

    if(!wPressed)
      sendSpeed(0);

  }


  // -------------------------
  // A
  // -------------------------

  if(e.code === "KeyA") {

    aPressed = false;

    fetch("/steerKey?dir=0");

  }


  // -------------------------
  // D
  // -------------------------

  if(e.code === "KeyD") {

    dPressed = false;

    fetch("/steerKey?dir=0");

  }

});


</script>

</body>

</html>

)rawliteral";


// =====================================================
// MOTOR OUTPUT
// =====================================================

void applyMotorPower(int speed) {

  int power = abs(speed);


  // ===================================================
  // MOTOR OFF
  // ===================================================

  if(power < MOTOR_DEADZONE) {

    ledcDetach(IN1);
    ledcDetach(IN2);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    return;
  }


  // ===================================================
  // FORWARD
  // ===================================================

  if(speed > 0) {

    analogWrite(IN1, power);

    digitalWrite(IN2, LOW);

  }


  // ===================================================
  // REVERSE
  // ===================================================

  else {

    analogWrite(IN2, power);

    digitalWrite(IN1, LOW);

  }

}


// =====================================================
// MOTOR UPDATE
// =====================================================

void updateMotor(float dt) {

  float desiredSpeed = 0;


  // ===================================================
  // NO THROTTLE
  // COAST
  // ===================================================

  if(targetSpeed == 0) {


    // Forward coasting

    if(currentSpeed > 0) {

      currentSpeed -=
        COAST_DECELERATION * dt;

      if(currentSpeed < 0)
        currentSpeed = 0;

    }


    // Reverse coasting

    else if(currentSpeed < 0) {

      currentSpeed +=
        COAST_DECELERATION * dt;

      if(currentSpeed > 0)
        currentSpeed = 0;

    }

  }


  // ===================================================
  // FORWARD
  // ===================================================

  else if(targetSpeed > 0) {

    desiredSpeed =
      constrain(
        targetSpeed,
        MIN_DRIVE,
        MAX_FORWARD
      );


    // If currently moving backward,
    // brake before going forward.

    if(currentSpeed < 0) {

      currentSpeed +=
        BRAKE_DECELERATION * dt;

      if(currentSpeed > 0)
        currentSpeed = 0;

    }


    // Accelerate

    else if(currentSpeed < desiredSpeed) {

      currentSpeed +=
        ACCELERATION * dt;

      if(currentSpeed > desiredSpeed)
        currentSpeed = desiredSpeed;

    }


    // Slow down if target becomes lower

    else if(currentSpeed > desiredSpeed) {

      currentSpeed -=
        ACCELERATION * dt;

      if(currentSpeed < desiredSpeed)
        currentSpeed = desiredSpeed;

    }

  }


  // ===================================================
  // REVERSE
  // ===================================================

  else {

    desiredSpeed = -MAX_REVERSE;


    // If moving forward,
    // brake before reversing.

    if(currentSpeed > 0) {

      currentSpeed -=
        BRAKE_DECELERATION * dt;

      if(currentSpeed < 0)
        currentSpeed = 0;

    }


    // Accelerate backward

    else if(currentSpeed > desiredSpeed) {

      currentSpeed -=
        REVERSE_ACCELERATION * dt;

      if(currentSpeed < desiredSpeed)
        currentSpeed = desiredSpeed;

    }

  }


  // ===================================================
  // MOTOR OUTPUT
  // ===================================================

  applyMotorPower(
    (int)currentSpeed
  );

}


// =====================================================
// STEERING UPDATE
// =====================================================

void updateSteering(float dt) {


  // ===================================================
  // KEYBOARD STEERING
  // ===================================================

  if(steeringInput != 0) {

    currentSteer +=
      STEER_SPEED *
      steeringInput *
      dt;

  }


  // ===================================================
  // PHONE STEERING
  // ===================================================

  else if(phoneSteering) {

    // Phone slider directly controls angle.
    // Nothing else changes it here.

  }


  // ===================================================
  // RETURN TO CENTER
  // ===================================================

  else {

    if(currentSteer < CENTER_ANGLE) {

      currentSteer +=
        STEER_RETURN_SPEED * dt;

      if(currentSteer > CENTER_ANGLE)
        currentSteer = CENTER_ANGLE;

    }


    else if(currentSteer > CENTER_ANGLE) {

      currentSteer -=
        STEER_RETURN_SPEED * dt;

      if(currentSteer < CENTER_ANGLE)
        currentSteer = CENTER_ANGLE;

    }

  }


  // ===================================================
  // STEERING LIMIT
  // ===================================================

  currentSteer =
    constrain(
      currentSteer,
      LEFT_ANGLE,
      RIGHT_ANGLE
    );


  // ===================================================
  // APPLY TRIM
  // ===================================================

  int finalSteer =
    (int)currentSteer +
    trimOffset;


  finalSteer =
    constrain(
      finalSteer,
      SERVO_MIN_LIMIT,
      SERVO_MAX_LIMIT
    );


  steeringServo.write(
    finalSteer
  );

}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // ===================================================
  // MOTOR
  // ===================================================

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // IMPORTANT:
  // Force motor completely OFF at startup.

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  targetSpeed = 0;
  currentSpeed = 0;


  // ===================================================
  // SERVO
  // ===================================================

  steeringServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  steeringServo.write(
    CENTER_ANGLE
  );

  currentSteer =
    CENTER_ANGLE;


  // ===================================================
  // WIFI
  // ===================================================

  WiFi.disconnect(true);

  WiFi.mode(WIFI_AP);

  WiFi.setTxPower(
    WIFI_POWER_19_5dBm
  );


  WiFi.softAP(
    ssid,
    password,
    1,
    0,
    4
  );


  Serial.print("SSID: ");
  Serial.println(ssid);

  Serial.print("IP: ");
  Serial.println(
    WiFi.softAPIP()
  );


  // ===================================================
  // HOME PAGE
  // ===================================================

  server.on("/", []() {

    server.send(
      200,
      "text/html",
      htmlPage
    );

  });


  // ===================================================
  // SPEED
  // ===================================================

  server.on("/setSpeed", []() {

    targetSpeed =
      server.arg("val").toInt();


    targetSpeed =
      constrain(
        targetSpeed,
        -MAX_REVERSE,
        MAX_FORWARD
      );


    server.send(
      200,
      "text/plain",
      "OK"
    );

  });


  // ===================================================
  // PHONE STEERING
  // ===================================================

  server.on("/setSteer", []() {

    currentSteer =
      server.arg("val").toFloat();


    currentSteer =
      constrain(
        currentSteer,
        LEFT_ANGLE,
        RIGHT_ANGLE
      );


    steeringInput = 0;

    phoneSteering = true;


    server.send(
      200,
      "text/plain",
      "OK"
    );

  });


  // ===================================================
  // KEYBOARD STEERING
  // ===================================================

  server.on("/steerKey", []() {

    steeringInput =
      server.arg("dir").toInt();


    steeringInput =
      constrain(
        steeringInput,
        -1,
        1
      );


    phoneSteering = false;


    server.send(
      200,
      "text/plain",
      "OK"
    );

  });


  // ===================================================
  // TRIM
  // ===================================================

  server.on("/setTrim", []() {

    trimOffset =
      server.arg("val").toInt();


    trimOffset =
      constrain(
        trimOffset,
        -25,
        25
      );


    server.send(
      200,
      "text/plain",
      "OK"
    );

  });


  // ===================================================
  // START SERVER
  // ===================================================

  server.begin();

  Serial.println(
    "Web server started!"
  );

}


// =====================================================
// LOOP
// =====================================================

void loop() {

  server.handleClient();


  unsigned long now =
    millis();


  static unsigned long lastTime =
    millis();


  float dt =
    (now - lastTime) /
    1000.0;


  lastTime = now;


  // Prevent giant time jumps

  dt =
    constrain(
      dt,
      0.001,
      0.05
    );


  updateMotor(dt);

  updateSteering(dt);

}
