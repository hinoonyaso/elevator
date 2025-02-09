#include <Servo.h>

// 서보 모터 객체 생성
Servo servo;

// 7세그먼트 및 LED 제어용 핀 설정
const int seg_datapin = 2;     // 7세그먼트 데이터 핀 (74HC595)
const int seg_latchpin = 3;    // 7세그먼트 래치 핀 (74HC595)
const int seg_clockpin = 4;    // 7세그먼트 클럭 핀 (74HC595)
const int led_datapin = 5;     // LED 데이터 핀 (74HC595)
const int led_latchpin = 6;    // LED 래치 핀 (74HC595)
const int led_clockpin = 7;    // LED 클럭 핀 (74HC595)

// 호출 LED, 호출 버튼, 및 층 위치 LED 핀 설정
const int call_LED[] = {10, 11, 12};    // 각 층의 호출 상태를 표시하는 GREEN LED
const int call_btn[] = {A1, A2, A3};      // 각 층 호출 버튼
const int floor_LED[] = {8, 9, 13};       // 엘리베이터의 현재 위치를 표시하는 RED LED

// 문(door) 관련 핀 설정
const int door_open_btn = A4;    // 문 열기 버튼 
const int door_close_btn = A5;   // 문 닫기 버튼 

int speed;
int speed_control;

// 공통 양극 7세그먼트 디스플레이 제어를 위한 숫자 패턴 배열
// 각 내부 배열은 세그먼트 a ~ g 의 ON(1) / OFF(0) 상태를 정의합니다.
// {A, G, F, E, D, C, B}
byte digits[] = {0x9F,0x25,0x0D};

int currentFloor = 0;             // 현재 층 (0: 1층, 1: 2층, 2: 3층)
bool callState[] = {false, false, false};  // 각 층의 호출 상태
bool doorOpen = false;            // 문의 열림 상태
bool elevatorMoving = false;      // 엘리베이터 이동 상태

unsigned long currentMillis;
unsigned long previousMillis = 0;
const long interval = 3000;  // 1초 지연

void setup() {
  Serial.begin(9600);             // 시리얼 통신 시작
  servo.attach(9);                // 서보 모터를 9번 핀에 연결
  
  // 7세그먼트 및 LED 제어용 핀들을 출력 모드로 설정
  pinMode(seg_datapin, OUTPUT);
  pinMode(seg_latchpin, OUTPUT);
  pinMode(seg_clockpin, OUTPUT);
  pinMode(led_datapin, OUTPUT);
  pinMode(led_latchpin, OUTPUT);
  pinMode(led_clockpin, OUTPUT);
  
  // 각 층의 호출 LED, 호출 버튼, 그리고 층 표시 LED 초기화
  for (int i = 0; i < 3; i++) {
    pinMode(call_LED[i], OUTPUT);        // 호출 상태 LED 출력 모드 설정
    pinMode(call_btn[i], INPUT_PULLUP);    // 호출 버튼 입력 모드 (내부 풀업 사용)
    pinMode(floor_LED[i], OUTPUT);         // 층 위치 LED 출력 모드 설정
  }
  
  // 문 제어 관련 핀 초기화
  pinMode(door_open_btn, INPUT);   // 문 열기 버튼 입력 모드 
  pinMode(door_close_btn, INPUT);   // 문 닫기 버튼 입력 모드
  
  Serial.println("Please press a floor button.");  // 초기 안내 메시지
  
  // 시작 시 현재 층을 1층으로 표시 (digits의 인덱스 1: 숫자 1)
  updateSegment(currentFloor);
}

// 7세그먼트 디스플레이에 숫자를 출력하는 함수
// digit: 0 ~ 9 값 (엘리베이터 층수 표시를 위해 1~3 사용)
void updateSegment(int digit) {
  // latch 핀을 LOW로 설정하여 데이터 전송 준비
  digitalWrite(seg_latchpin, LOW);
  
  // 7세그먼트 7개의 세그먼트를 제어할 바이트 생성
  // digits 배열에서 해당 digit의 패턴을 읽어, 각 비트를 outByte에 설정
  // 74HC595에 outByte를 전송 (MSBFIRST는 최상위부터, LSBFIRST는 최하위부터 전송 시작)
  shiftOut(seg_datapin, seg_clockpin, LSBFIRST, digits[digit]);
  
  // latch 핀을 HIGH로 설정하여 출력 업데이트
  digitalWrite(seg_latchpin, HIGH);
}

// 호출된 층으로 엘리베이터 이동 함수
void moveElevator() 
{
  for (int i = 0; i < 3; i++) 
  {
    if (callState[i]) // 호출 상태가 true인 층이 있으면
    {  
      elevatorMoving = true;  // 엘리베이터 이동 시작
      Serial.print("Moving to floor: ");
      Serial.println(i + 1);
      updateLEDs(i);        // 엘리베이터 위치 LED 업데이트
      callState[i] = false; // 호출 상태 초기화
      updateLEDProgressive(i); // LED를 순차적으로 점등
      // 엘리베이터가 도착하면 7세그먼트 디스플레이에 해당 층 표시 (층 번호는 1~3)
      updateSegment(i);
      
      digitalWrite(call_LED[i], LOW); // 호출 LED 끄기
      elevatorMoving = false; // 엘리베이터 이동 종료
      openDoor();           // 도착 후 문 열기
      delay(2000);
      closeDoor();           // 2초 대기 후 문 닫기
      currentFloor=i;
      previousMillis = millis();
    }
  }
}

void move1F() 
{
    elevatorMoving = true;  // 엘리베이터 이동 시작
    updateLEDs(0);        // 엘리베이터 위치 LED 업데이트
    callState[0] = false; // 호출 상태 초기화
    updateLEDProgressive(0); // LED를 순차적으로 점등
    // 엘리베이터가 도착하면 7세그먼트 디스플레이에 해당 층 표시 (층 번호는 1~3)
    updateSegment(0);
      
    digitalWrite(call_LED[0], LOW); // 호출 LED 끄기
    elevatorMoving = false; // 엘리베이터 이동 종료          // 2초 대기 후 문 닫기
    currentFloor=0;
}

// 현재 엘리베이터 위치를 표시하는 LED 업데이트 함수
void updateLEDs(int targetFloor) 
{
  for (int i = 0; i < 3; i++) 
  {
    digitalWrite(floor_LED[i], (i == targetFloor));  // targetFloor에 해당하는 LED만 켜기
  }
}

void updateLEDProgressive(int floor) 
{
  int startLED, endLED, step;

  if (currentFloor == 0) 
  {
    startLED = 0;
    endLED = (floor == 1) ? 3 : 6;
    step = 1;
  } 
  else if (currentFloor == 1) 
  {
    startLED = (floor == 0) ? 3 : 3;
    endLED = (floor == 0) ? 0 : 6;
    step = (floor == 0) ? -1 : 1;
  } 
  else 
  {
    startLED = 6;
    endLED = (floor == 1) ? 3 : 0;
    step = -1;
  }

  for (int i = startLED; (step > 0) ? (i <= endLED) : (i >= endLED); i += step) 
  {
    digitalWrite(led_latchpin, LOW);
    byte ledData = (1 << i);
    shiftOut(led_datapin, led_clockpin, MSBFIRST, ledData);
    if(currentFloor == 0 && floor == 2 && i==3)
    {
      updateSegment(1);
    }
    if(currentFloor == 2 && floor == 0 && i==3)
    {
      updateSegment(1);
    }
    digitalWrite(led_latchpin, HIGH);
    delay(speed);
  }
}

// 문 열기 함수: 문 열림 상태를 LED 및 시리얼 모니터에 표시
void openDoor() 
{
  Serial.println("Door opened");
  doorOpen = true;
  servo.write(90);
}

// 문 닫기 함수: 문 닫힘 상태를 LED 및 시리얼 모니터에 표시
void closeDoor() 
{
  Serial.println("Door closed");
  doorOpen = false;
  servo.write(0);
}

void loop() {
  currentMillis=millis();
  if (currentMillis - previousMillis >= interval && !elevatorMoving && currentFloor!=0) 
  {
    move1F();  // 층을 업데이트
  }
  // 각 층의 호출 버튼 상태 확인 (버튼을 누르면 호출 상태 토글)
  for (int i = 0; i < 3; i++) 
  {
    if (digitalRead(call_btn[i]) == HIGH) // 버튼 누름 확인
    {  
      delay(50); // 디바운스 처리
      if (digitalRead(call_btn[i]) == HIGH) {
        callState[i] = !callState[i];                // 호출 상태 토글 (호출/취소)
        digitalWrite(call_LED[i], callState[i]);       // 호출 LED 상태 변경
        Serial.print("Floor ");
        Serial.print(i + 1);
        Serial.println(" called");
      }
    }
  }
  
  // 엘리베이터가 이동 중이 아닐 때만 문 제어 버튼 확인
  if (!elevatorMoving) {
    if (digitalRead(door_open_btn) == HIGH) {   // 문 열기 버튼 누름
      openDoor();
    }
    if (digitalRead(door_close_btn) == HIGH) {  // 문 닫기 버튼 누름
      closeDoor();
    }
  }
  speed_control = analogRead(A0);
  speed = map(speed_control,0,1024,100,2000);
  
  // 호출된 층이 있으면 엘리베이터 이동 처리
  moveElevator();
}
