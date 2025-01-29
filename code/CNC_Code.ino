#include <Servo.h>   // Importação biblioteca servo motor;
#include <AFMotor.h> // Importação biblioteca shield L293D;
#define LINE_BUFFER_LENGTH 512

char STEP = MICROSTEP;

const int penZUp = 130;            // Posição do servo (eixo z) para cima;
const int penZDown = 83;           // Posição do servo (eixo z) para baixo;
const int penServoPin = 10;        // Pino 10 PWM para controlar servo motor;
const int stepsPerRevolution = 48; // Configuração padrão para motores de passo de drivers de CD-ROM;

Servo penServo; // Criando objeto para controlar o servo motor;

AF_Stepper myStepperY(stepsPerRevolution, 1); // Inicializando motores usando os pinos do Arduino para a shield L293D(Eixo Y);
AF_Stepper myStepperX(stepsPerRevolution, 2); // Inicializando motores usando os pinos do Arduino para a shield L293D(Eixo X);

// Estrutura das variáveis globais que serão utilizadas no código;
struct point
{
  float x;
  float y;
  float z;
};

// Posição atual dos atuadores
struct point actuatorPos;

//  Nessa estrutura é definida as configurações de desenho - só inicialização se estiverem tudo corretamente;
float StepInc = 1;
int StepDelay = 1;
int LineDelay = 1;
int penDelay = 50;

// Configura os passos do motor para "andar" 1 milímetro;
// Teste para ir 100 etapas. É medido o comprimento da linha feita pela máquina;
// A medida deve ser transformada em milímetros;
float StepsPerMillimeterX = 100.0;
float StepsPerMillimeterY = 100.0;

// O resultado da medida define os limites da área de desenho;
// Nessa estrutura é definida o ponto mínimo e o ponto máximo que a máquina pode percorrer em cada eixo;
float Xmin = 0;
float Xmax = 40;
float Ymin = 0;
float Ymax = 40;
float Zmin = 0;
float Zmax = 1;

float Xpos = Xmin; // Reseta para posição inicial o eixo X;
float Ypos = Ymin; // Reseta para posição inicial o eixo Y;
float Zpos = Zmax; // Reseta para posição inicial o eixo Z;

boolean verbose = false;

//  Para mover os eixos a máquina precisa interpretar o gcode:
//  G1 para mover;
//  G4 P150 (esperar 150ms);
//  M300 S30 (caneta para baixo);
//  M300 S50 (caneta para cima);
//  M18 Desliga os drivers
//  As informações entre "()" são ignoradas;
//  Descarta qualquer outro comando "!";

/*******************************
 * Inicialização do void setup *
 *******************************/

/* void setup(): é executada apenas quando começa o programa
 * e serve para configurar os pinos da placa e
 * estabecer a comunicação serial com um computador*/

void setup()
{

  Serial.begin(9600);

  penServo.attach(penServoPin);
  penServo.write(penZUp);
  delay(100);

  // Velocidade dos motores que controlam os eixos (x, y);
  myStepperX.setSpeed(600);
  myStepperY.setSpeed(600);

  //  Notificações no monitor serial do arduino!!!
  Serial.println("D&M - CNC conectada e pronta para funcionar!");
}

/**************************************************
 * Inicialização do void loop() -- loop principal *
 **************************************************/

void loop()
{

  // Pequeno delay de partida;
  // Declaração de algumas variavéis;
  delay(100);
  char line[LINE_BUFFER_LENGTH];
  char c;
  int lineIndex;
  bool lineIsComment, lineSemiColon;

  lineIndex = 0;
  lineSemiColon = false;
  lineIsComment = false;

  // Enquanto a condição for verdadeira (true) executar o seguinte bloco:
  while (1)
  {

    // Controla a recepção dos dados e os organiza
    while (Serial.available() > 0)
    {
      c = Serial.read();
      if ((c == '\n') || (c == '\r'))
      { // Indica que o fim da linha foi alcançado e pula uma linha;
        if (lineIndex > 0)
        {                         // Caso a linha esteja completa EXECUTE a ação;
          line[lineIndex] = '\0'; // Termina a leitura de uma string
          if (verbose)
          {
            Serial.print("Recebido : ");
            Serial.println(line);
          }
          processIncomingLine(line, lineIndex);
          lineIndex = 0;
        }
        else
        {
          // Pular linha de comentario caso esteja vazio. Pular bloco.
        }
        lineIsComment = false;
        lineSemiColon = false;
        Serial.println("ok - D&M");
      }
      else
      {
        if ((lineIsComment) || (lineSemiColon))
        {
          if (c == ')')
            lineIsComment = false; // Ignora o comentario, retoma a linha;
        }
        else
        {
          if (c <= ' ')
          { // Exclui os espaços em branco e controla os caracteres;
          }
          else if (c == '/')
          { // Ignora os caracteres;
          }
          else if (c == '(')
          { // Ativa a sinalização dos comentarios e ignora todos os caracteres até ')';.
            lineIsComment = true;
          }
          else if (c == ';')
          {
            lineSemiColon = true;
          }
          else if (lineIndex >= LINE_BUFFER_LENGTH - 1)
          {
            Serial.println("ERROR - Na linha do Buffer");
            lineIsComment = false;
            lineSemiColon = false;
          }
          else if (c >= 'a' && c <= 'z')
          { // Maiúscula ou minúscula
            line[lineIndex++] = c - 'a' + 'A';
          }
          else
          {
            line[lineIndex++] = c;
          }
        }
      }
    }
  }
}

void processIncomingLine(char *line, int charNB)
{
  int currentIndex = 0;
  char buffer[64];
  struct point newPos;

  newPos.x = 0.0; // Nova(reseta) posição eixo x
  newPos.y = 0.0; // Nova(reseta) posição eixo y

  //  Para mover os eixos a máquina precisa interpretar o gcode:
  //  G1 para mover;
  //  G4 P300 (esperar 150ms);
  //  M300 S30 (caneta para baixo);
  //  M300 S50 (caneta para cima);
  //  M18 Desliga os drivers
  //  As informações entre "()" são ignoradas;
  //  Descarta qualquer outro comando "!";

  while (currentIndex < charNB)
  {
    switch (line[currentIndex++])
    { // Selecionar um comando se houver;
    case 'U':
      penUp();
      break;
    case 'D':
      penDown();
      break;
    case 'G':
      buffer[0] = line[currentIndex++]; // Apenas funciona com comando de 2 dígitos
      buffer[1] = '\0';
      // buffer[1] = line[ currentIndex++ ];
      // buffer[2] = '\0';

      switch (atoi(buffer))
      {       // Seleciona o gcode
      case 0: // G00 & G01 - Movimento
      case 1:
        // Se X estiver antes do Y:
        char *indexX = strchr(line + currentIndex, 'X'); // Pega a posição de X na string (caso tenha)
        char *indexY = strchr(line + currentIndex, 'Y'); // Pega a posição de Y na string (caso tenha)
        if (indexY <= 0)
        {
          newPos.x = atof(indexX + 1);
          newPos.y = actuatorPos.y;
        }
        else if (indexX <= 0)
        {
          newPos.y = atof(indexY + 1);
          newPos.x = actuatorPos.x;
        }
        else
        {
          newPos.y = atof(indexY + 1);
          indexY = '\0';
          newPos.x = atof(indexX + 1);
        }
        drawLine(newPos.x, newPos.y);
        // Serial.println("ok");
        actuatorPos.x = newPos.x;
        actuatorPos.y = newPos.y;
        break;
      }
      break;
    case 'M':
      buffer[0] = line[currentIndex++]; // Funciona apenas com comandos de 3 dígitos
      buffer[1] = line[currentIndex++];
      buffer[2] = line[currentIndex++];
      buffer[3] = '\0';
      switch (atoi(buffer))
      {
      case 300:
      {
        char *indexS = strchr(line + currentIndex, 'S');
        float Spos = atof(indexS + 1);
        //  Serial.println("ok");
        if (Spos == 30)
        {
          penDown();
        }
        if (Spos == 50)
        {
          penUp();
        }
        break;
      }
      case 114:
        Serial.print("Posição Absoluta: X = ");
        Serial.print(actuatorPos.x);
        Serial.print("  -  Y = ");
        Serial.println(actuatorPos.y);
        break;
      default:
        Serial.print("Comando não reconhecido : M");
        Serial.println(buffer);
      }
    }
  }
}

/*
 * int (x1;y1) : Coordenada Inicial
 * int (x2;y2) : Coordenada Final
 */
void drawLine(float x1, float y1)
{

  if (verbose)
  {
    Serial.print("fx1, fy1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }

  //  Trazer as instruções dentro dos limites da base
  if (x1 >= Xmax)
  {
    x1 = Xmax;
  }
  if (x1 <= Xmin)
  {
    x1 = Xmin;
  }
  if (y1 >= Ymax)
  {
    y1 = Ymax;
  }
  if (y1 <= Ymin)
  {
    y1 = Ymin;
  }

  if (verbose)
  {
    Serial.print("Xpos, Ypos: ");
    Serial.print(Xpos);
    Serial.print(",");
    Serial.print(Ypos);
    Serial.println("");
  }

  if (verbose)
  {
    Serial.print("x1, y1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }

  //  Convertendo as coordenadas em passos
  x1 = (int)(x1 * StepsPerMillimeterX);
  y1 = (int)(y1 * StepsPerMillimeterY);
  float x0 = Xpos;
  float y0 = Ypos;

  // Calculando as mudanças para as coordenadas
  long dx = abs(x1 - x0);
  long dy = abs(y1 - y0);
  int sx = x0 < x1 ? StepInc : -StepInc;
  int sy = y0 < y1 ? StepInc : -StepInc;

  long i;
  long over = 0;

  if (dx > dy)
  {
    for (i = 0; i < dx; ++i)
    {
      myStepperX.onestep(sx, STEP);
      over += dy;
      if (over >= dx)
      {
        over -= dx;
        myStepperY.onestep(sy, STEP);
      }
      delay(StepDelay);
    }
  }
  else
  {
    for (i = 0; i < dy; ++i)
    {
      myStepperY.onestep(sy, STEP);
      over += dx;
      if (over >= dy)
      {
        over -= dy;
        myStepperX.onestep(sx, STEP);
      }
      delay(StepDelay);
    }
  }

  if (verbose)
  {
    Serial.print("dx, dy:");
    Serial.print(dx);
    Serial.print(",");
    Serial.print(dy);
    Serial.println("");
  }

  if (verbose)
  {
    Serial.print("Going to (");
    Serial.print(x0);
    Serial.print(",");
    Serial.print(y0);
    Serial.println(")");
  }

  //  Atraso antes que as próximas linhas sejam enviadas
  delay(LineDelay);
  // Atualizar a posição
  Xpos = x1;
  Ypos = y1;
}

//  Levanta a Caneta
void penUp()
{
  penServo.write(penZUp);
  delay(penDelay);
  Zpos = Zmax;
  digitalWrite(15, LOW);
  digitalWrite(16, HIGH);
  if (verbose)
  {
    Serial.println("Pen up!");
  }
}
//   Abaixa a Caneta
void penDown()
{
  penServo.write(penZDown);
  delay(penDelay);
  Zpos = Zmin;
  digitalWrite(15, HIGH);
  digitalWrite(16, LOW);
  if (verbose)
  {
    Serial.println("Pen down.");
  }
}
