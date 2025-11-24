#include "SimuladorGLWidget.h"

#include <iostream>

SimuladorGLWidget::SimuladorGLWidget (QWidget* parent) : QOpenGLWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);  // per rebre events de teclat
  xClick = yClick = 0;
  DoingInteractive = NONE;
  animationTimer = new QTimer(this);
  connect(animationTimer, &QTimer::timeout, this, &SimuladorGLWidget::updateAnimation);
  //animationTimer->start(16);
  robotCamera = false;  // Initialize to false - start with normal camera
}

SimuladorGLWidget::~SimuladorGLWidget ()
{
  if (program != NULL)
    delete program;
}


void SimuladorGLWidget::initializeGL ()
{
  // Initialize robots map with tuple (x, y, direction)
  robots = std::map<int,std::tuple<float,float,int>>();
  
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();  

  glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_STENCIL_TEST);  // Enable stencil testing

  carregaShaders();
  iniEscena ();
  iniCamera ();
}

void SimuladorGLWidget::modifyselectedRobotID(QString robotID){
  makeCurrent();
    if(robotID.isEmpty()) {
      selectedRObotID = -1;
      std::cout << "Robot seleccionat amb ID: " << selectedRObotID << "\n";
      return;
    }
    selectedRObotID=robotID.toInt();
    if(robotCamera) {
      viewTransform();
    }
    update();
  std::cout << "Robot seleccionat amb ID: " << selectedRObotID << "\n";
}

void SimuladorGLWidget::updateAnimation() {
  makeCurrent();
  for (auto& robot : robots) {
      float& x = std::get<0>(robot.second);
      float& y = std::get<1>(robot.second);
      int& dir = std::get<2>(robot.second);
      if (rand() % 100 < 10) {
          dir = rand() % 4;  // Pick random direction (0-3)
      }
      // Move based on direction
      switch(dir) {
          case 0: // right
            x += Speed/60.f;
            if (x > 29.0f) {
                x = 29.0f;
                dir = (dir + 1) % 4; // turn to next direction
            }
            break;
          case 1: // up
            y += Speed/60.f;
            if (y > 29.0f) {
                y = 29.0f;
                dir = (dir + 1) % 4;
            }
            break;
          case 2: // left
            x -= Speed/60.f;
            if (x < 0.0f) {
                x = 0.0f;
                dir = (dir + 1) % 4;
            }
            break;
          case 3: // down
            y -= Speed/60.f;
            if (y < 0.0f) {
                y = 0.0f;
                dir = (dir + 1) % 4;
            }
            break;
      }
  }
  if(robotCamera) {
      viewTransform();
  }
  update();
}

void SimuladorGLWidget::iniEscena ()
{
  creaBuffersModels();
  creaBuffersTerra();

  // inicialitzem tots els possibles objectes (blocs de Lego) com a "buits" --> no es pinten ni tenen valors guardats més que els per defecte
  for (int i = 0; i < NUM_BRICKS; i++)
  {
  	pintarBricks[i] = false;  	
  	brickModelIndex[i] = 1;
  	brickTGs[i] = glm::mat4(1);
  	brickColors[i] = glm::vec3(1,1,1);
  }
  
  // inicialitzem l'objecte actual (el bloc de Lego) a editar
  currentBrickObjectIndex = 0;
  currentBrickModelIndex = 1;

  // inicialitzem el color actual
  currentColor = white;	

  // Inicialitzem els paràmetres de l'escena amb valors arbitraris
  centreEsc = glm::vec3 (15, 0, 15);
  radiEsc = sqrt(20*20+10*10+2*2);
}

void SimuladorGLWidget::iniCamera ()
{
  angleY = M_PI/4.0f;  // 45 degrees initial angle
  angleX = M_PI/4.0f;
  ra = float(width())/float(height());
  fov = float(M_PI/3.0);  // 60 degrees
  zn = 0.1f;  // Use smaller near plane
  zf = 3*radiEsc;
  orto = false;
  projectTransform();
  viewTransform();
}

void SimuladorGLWidget::afegirRobot(int x, int y) {
    makeCurrent();
    std::cout << "Afegir robot a la posició: (" << x << ", " << y << ")\n";
    robots[nextRobotID] = std::make_tuple(float(x), float(y), 0);  // Start facing right
    emit robotAfegit(nextRobotID);
    nextRobotID++;
    update();
}


void SimuladorGLWidget::eliminarRobot(int robotID){
  makeCurrent();
  robots.erase(robotID);
  std::cout << "Eliminar robot amb ID: " << robotID << "\n";
  update();
}

void SimuladorGLWidget::modelTransforRobot (int id, float x, float y,int dir){
    glm::mat4 TG(1.0f);
    
    // Position translation
    TG = glm::translate(TG, glm::vec3(x, 0.f, y));
    // Rotation based on direction
    float angle = 0.0f;
    angle = glm::radians(90.0f * dir); // 90 degrees per direction
    TG = glm::rotate(TG, angle, glm::vec3(0, 1, 0));
  
    
    // Model adjustments
    TG = glm::translate(TG, glm::vec3(-centreCapsaModels[1].x, -minY[1], -centreCapsaModels[1].z));
    
    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &TG[0][0]);
}
void SimuladorGLWidget::paintGL ()
{
// En cas de voler canviar els paràmetres del viewport, descomenteu la crida següent i
// useu els paràmetres que considereu (els que hi ha són els de per defecte)
//  glViewport (0, 0, ample, alt);
  
  // Esborrem el frame-buffer i el depth-buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // Draw ground first
  glUniform3fv(colorLoc, 1, &white[0]);
  
  glStencilMask(0x00); // Don't write to stencil buffer
  glBindVertexArray(VAO_Terra);
  modelTransformGround();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


  // First pass: Draw all non-selected robots normally
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0x00);
  for (const auto& robot : robots) {
        if (robot.first == selectedRObotID) continue;
        
        const float x = std::get<0>(robot.second);
        const float y = std::get<1>(robot.second);
        const int dir = std::get<2>(robot.second);

        modelTransforRobot(robot.first, x, y, dir);
        glBindVertexArray(VAO_models[1]);
        glDrawArrays(GL_TRIANGLES, 0, models[1].faces().size() * 3);
    }

    // If we have a selected robot, draw it with outline
    if (selectedRObotID != -1) {
        auto it = robots.find(selectedRObotID);
        if (it != robots.end()) {
            const float x = std::get<0>(it->second);
            const float y = std::get<1>(it->second);
            const int dir = std::get<2>(it->second);



            // First render: Write to stencil buffer
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            
            modelTransforRobot(selectedRObotID, x, y, dir);
            glBindVertexArray(VAO_models[1]);
            glDrawArrays(GL_TRIANGLES, 0, models[1].faces().size() * 3);

            // Second render: Draw slightly scaled outline
            if(!robotCamera){
              glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
              glStencilMask(0x00);
              glDisable(GL_DEPTH_TEST);

              // Create scaled transform for outline
              glm::vec3 transDesp = glm::vec3(0, centreCapsaModels[1].y - minY[1], 0);
              glm::mat4 TG(1.0f);
              TG = glm::translate(TG, glm::vec3(x, 0.f, y));
              float angle = glm::radians(90.0f * dir);
              TG = glm::rotate(TG, angle, glm::vec3(0,1,0));
              TG = glm::translate(TG, transDesp);
              TG = glm::scale(TG, glm::vec3(1.1f)); // Scale up for outline
              TG = glm::translate(TG, glm::vec3(-centreCapsaModels[1].x, -centreCapsaModels[1].y, -centreCapsaModels[1].z));
              
              // Draw outline in red
              glm::vec3 outlineColor(1.0f, 0.0f, 0.0f); // Red outline
              glUniform3fv(colorLoc, 1, &outlineColor[0]);
              glUniformMatrix4fv(transLoc, 1, GL_FALSE, &TG[0][0]);
              glDrawArrays(GL_TRIANGLES, 0, models[1].faces().size() * 3);
            }
            // Restore states
            glEnable(GL_DEPTH_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glUniform3fv(colorLoc, 1, &white[0]);
        }
    }

    // Draw camera position marker (if in third person and a robot is selected)
    if (selectedRObotID != -1 && !robotCamera) {
        auto it = robots.find(selectedRObotID);
        if (it != robots.end()) {
            const float x = std::get<0>(it->second);
            const float y = std::get<1>(it->second);
            const int dir = std::get<2>(it->second);

            // Calculate camera position
            float transDesp = centreCapsaModels[1].y - minY[1];
            float t2 = centreCapsaModels[1].x - minX[1];
            glm::vec3 robotPos(x, transDesp, y+t2);

            // Draw small cube at camera position
            glm::mat4 TG(1.0f);
            TG = glm::translate(TG, robotPos);
            TG = glm::scale(TG, glm::vec3(0.2f)); // Small cube
            
            glUniformMatrix4fv(transLoc, 1, GL_FALSE, &TG[0][0]);
            glm::vec3 debugColor(0.0f, 1.0f, 0.0f); // Green color for visibility
            glUniform3fv(colorLoc, 1, &debugColor[0]);
            
            // Use the first model (cube) to represent camera position
            glBindVertexArray(VAO_models[1]);
            glDrawArrays(GL_TRIANGLES, 0, models[1].faces().size() * 3);
            
            // Restore white color
            glUniform3fv(colorLoc, 1, &white[0]);
        }
    }
    
  glBindVertexArray(0);
}

void SimuladorGLWidget::resizeGL (int w, int h) 
{
#ifdef __APPLE__
  // Aquest codi és necessari únicament per a MACs amb pantalla retina.
  GLint vp[4];
  glGetIntegerv (GL_VIEWPORT, vp);
  ample = vp[2];
  alt = vp[3];
#else
  ample = w;
  alt = h;
#endif

  ra = float(ample)/float(alt);
  projectTransform();
}



void SimuladorGLWidget::modelTransformGround()
{
  glm::mat4 TG(1.f);
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}




void SimuladorGLWidget::projectTransform ()
{
  if(!orto) {
    float baseAngle = 2.f*asin(1.f/2.f);  // ~60 degrees
    if(ra < 1.0f) {
      fov = 2.f*atan(tan(baseAngle*0.5f)/ra);
    } else {
      fov = baseAngle;
    }
    glm::mat4 Proj = glm::perspective(fov, ra, zn, zf);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &Proj[0][0]);
  } else {
    float left,right,top,botom;
    left=-radiEsc;
    right=-left;
    top=radiEsc;
    botom=-top;
    if(ra>1){
      left=left*ra;
      right=right*ra;
    }else if(ra<1){
      botom=botom/ra;
      top=top/ra;
    }
    glm::mat4 Proj;  // Matriu de projecció
    Proj=glm::ortho(left,right,botom,top,0.f,500.f);
    glUniformMatrix4fv (projLoc, 1, GL_FALSE, &Proj[0][0]);
  }
}

void SimuladorGLWidget::viewTransform ()
{
    if (robotCamera && selectedRObotID != -1) {
        auto it = robots.find(selectedRObotID);
        if (it != robots.end()) {
            const float x = std::get<0>(it->second);
            const float y = std::get<1>(it->second);
            const int dir = std::get<2>(it->second);
            
            // Calculate forward direction based on robot rotation
            //float angle = glm::radians(90.0f * dir);
            //glm::vec3 forward(sin(angle), 0, cos(angle));

            // Calculate robot's actual height from ground
            float robotHeight = centreCapsaModels[1].y - minY[1];
            float t2 = centreCapsaModels[1].x - minX[1];
            // Position camera at robot position
            glm::vec3 robotPos(x, robotHeight, y+t2);
            glm::vec3 cameraPos = robotPos;
            
            // Look point is ahead of robot
            glm::vec3 lookAt = cameraPos + glm::vec3(0,0,1)*3.f;
            
            View = glm::lookAt(
                cameraPos,          
                lookAt,             
                glm::vec3(0,1,0)    
            );

            // Set near plane based on camera height to prevent seeing through floor
            zn = robotHeight * 0.8f;  // 80% of camera height
            projectTransform();
        }
    } else if (!orto) {
        // Regular perspective camera
        float angleeX_2 = angleY + glm::radians(180.0f);
        View = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -2*radiEsc));
        View = glm::rotate(View, angleX, glm::vec3(1, 0, 0));
        View = glm::rotate(View, -angleeX_2, glm::vec3(0, 1, 0));
        View = glm::translate(View, -centreEsc);
    } else {
        // Orthographic top-down view
        zn=0.1f;
        View = glm::lookAt(
            glm::vec3(centreEsc.x, 2*radiEsc, centreEsc.z),
            glm::vec3(centreEsc.x, 0, centreEsc.z),
            glm::vec3(1, 0, 0)
        );
    }
    
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &View[0][0]);
}
void SimuladorGLWidget::keyPressEvent(QKeyEvent* event) 
{
  makeCurrent();
  switch (event->key()) {
    case Qt::Key_M: { // al prèmer la tecla M canviem el model del bloc que estem editant
	    	currentBrickModelIndex++;
	    	if (currentBrickModelIndex == NUM_MODELS)
		    	currentBrickModelIndex = 1;
	    	break;
    	}
    case Qt::Key_B: { // al prèmer la tecla B canviem el color del bloc que estem editant
  		if (currentColor == white)
	    	  currentColor = glm::vec3(0,0,1);
		else 
		  currentColor = white;
	    	break;
    	}
    case Qt::Key_Q: {
      // Toggle between perspective and orthographic
      orto = !orto;
      projectTransform();
      viewTransform();
      update();
      break;
    }
    case Qt::Key_Z: {
        if (selectedRObotID != -1) {
            robotCamera = !robotCamera;
            viewTransform();
            update();
        }
        break;
    }
    default: event->ignore(); break;
  }
  update();
}

void SimuladorGLWidget::mousePressEvent (QMouseEvent *e)
{
  xClick = e->x();
  yClick = e->y();

  if (e->button() & Qt::LeftButton &&
      ! (e->modifiers() & (Qt::ShiftModifier|Qt::AltModifier|Qt::ControlModifier)))
  {
    DoingInteractive = ROTATE;
  }
}

void SimuladorGLWidget::mouseReleaseEvent( QMouseEvent *)
{
  DoingInteractive = NONE;
}
void SimuladorGLWidget::mouseMoveEvent(QMouseEvent *e)
{
  makeCurrent();
if ((DoingInteractive == ROTATE) && !orto)
  {
    // Fem la rotació
    angleY += (e->x() - xClick) * M_PI / ample;
    angleX += (e->y() - yClick) * M_PI / alt;
    viewTransform ();
  }
   if ((DoingInteractive == ROTATE) && orto)
  {
    // Fem la rotació
    angleY += (e->x() - xClick) * M_PI / ample;
    viewTransform ();
  }

  xClick = e->x();
  yClick = e->y();

  update ();
}

void SimuladorGLWidget::calculaCapsaModel (Model &p, float &escala, float ampladaDesitjada, glm::vec3 &centreCapsa,float &minY,float &minX,float &minZ)
{
  // Càlcul capsa contenidora i valors transformacions inicials
  float minx, miny, minz, maxx, maxy, maxz;
  minx = maxx = p.vertices()[0];
  miny = maxy = p.vertices()[1];
  minz = maxz = p.vertices()[2];
  for (unsigned int i = 3; i < p.vertices().size(); i+=3)
  {
    if (p.vertices()[i+0] < minx)
      minx = p.vertices()[i+0];
    if (p.vertices()[i+0] > maxx)
      maxx = p.vertices()[i+0];
    if (p.vertices()[i+1] < miny)
      miny = p.vertices()[i+1];
    if (p.vertices()[i+1] > maxy)
      maxy = p.vertices()[i+1];
    if (p.vertices()[i+2] < minz)
      minz = p.vertices()[i+2];
    if (p.vertices()[i+2] > maxz)
      maxz = p.vertices()[i+2];
  }
  
  escala = ampladaDesitjada/(maxx-minx);
  centreCapsa = glm::vec3((minx+maxx)/2.0f,(miny+maxy)/2.0f,(minz+maxz)/2.0f);
  minY=miny;
  minX=minx;
  minZ=minz;
}

void SimuladorGLWidget::creaBuffersModels ()
{  
  // Creació de VAos i VBOs per pintar els models
  glGenVertexArrays(NUM_MODELS, &VAO_models[0]);
  
  
  for (int i = 0; i < NUM_MODELS; i++)
  {	
      	  // Càrrega del models
  	  models[i].load("./models/"+objNames[i]);
  
	  // Calculem la capsa contenidora del model
	  calculaCapsaModel (models[i], escalaModels[i], ampladesDesitjades[i], centreCapsaModels[i],minY[i],minX[i],minZ[i]);
  
	  glBindVertexArray(VAO_models[i]);

	  // Creació dels buffers del model fantasma
	  GLuint VBO[6];
	  // Buffer de posicions
	  glGenBuffers(6, VBO);
	  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*models[i].faces().size()*3*3, models[i].VBO_vertices(), GL_STATIC_DRAW);

	  // Activem l'atribut vertexLoc
	  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	  glEnableVertexAttribArray(vertexLoc);

	  // Buffer de normals
	  glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*models[i].faces().size()*3*3, models[i].VBO_normals(), GL_STATIC_DRAW);

	  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	  glEnableVertexAttribArray(normalLoc);

	  // En lloc del color, ara passem tots els paràmetres dels materials
	  // Buffer de component ambient
	  glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*models[i].faces().size()*3*3, models[i].VBO_matamb(), GL_STATIC_DRAW);

	  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	  glEnableVertexAttribArray(matambLoc);

	  // Buffer de component difusa
	  glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*models[i].faces().size()*3*3, models[i].VBO_matdiff(), GL_STATIC_DRAW);

	  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	  glEnableVertexAttribArray(matdiffLoc);

	  // Buffer de component especular
	  glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*models[i].faces().size()*3*3, models[i].VBO_matspec(), GL_STATIC_DRAW);

	  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	  glEnableVertexAttribArray(matspecLoc);

	  // Buffer de component shininness
	  glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*models[i].faces().size()*3, models[i].VBO_matshin(), GL_STATIC_DRAW);

	  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
	  glEnableVertexAttribArray(matshinLoc);
  }
  
  glBindVertexArray (0);
}
void SimuladorGLWidget::iniMaterialTerra ()
{
  // Donem valors al material del terra
  amb = glm::vec3(0.2,0.1,0.2);
  diff = glm::vec3(0.6,0.2,0.6);
  spec = glm::vec3(0,0,0);
  shin = 500;
}

void SimuladorGLWidget::creaBuffersTerra ()
{
  // VBO amb la posició dels vèrtexs
  glm::vec3 posterra[] = {
	glm::vec3(-1.0, 0.0, 30.0),
	glm::vec3(30.0, 0.0, 30.0),
	glm::vec3(-1.0, 0.0, 0.0),
	glm::vec3(30.0, 0.0, -1.0),
  }; 

  // VBO amb la normal de cada vèrtex
  glm::vec3 norm (0,1,0);
  glm::vec3 normterra[6] = {
	norm, norm, norm, norm
  };

  // inicialitzem el material del terra
  iniMaterialTerra();

  // Fem que aquest material afecti a tots els vèrtexs per igual
  glm::vec3 matamb[] = {
	amb, amb, amb, amb
  };
  glm::vec3 matdiff[6] = {
	diff, diff, diff, diff
  };
  glm::vec3 matspec[6] = {
	spec, spec, spec, spec
  };
  float matshin[6] = {
	shin, shin, shin, shin
  };

  // Creació del Vertex Array Object del terra
  glGenVertexArrays(1, &VAO_Terra);
  glBindVertexArray(VAO_Terra);

  GLuint VBO_Terra[6];
  glGenBuffers(6, VBO_Terra);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(posterra), posterra, GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normterra), normterra, GL_STATIC_DRAW);

  // Activem l'atribut normalLoc
  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matamb), matamb, GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matdiff), matdiff, GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matspec), matspec, GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matshin), matshin, GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  glBindVertexArray(0);
}


void SimuladorGLWidget::carregaShaders() 
{
  // Creem els shaders per al fragment shader i el vertex shader
  QOpenGLShader fs (QOpenGLShader::Fragment, this);
  QOpenGLShader vs (QOpenGLShader::Vertex, this);
  // Carreguem el codi dels fitxers i els compilem
  fs.compileSourceFile("./shaders/basicLlumShader.frag");
  vs.compileSourceFile("./shaders/basicLlumShader.vert");
  // Creem el program
  program = new QOpenGLShaderProgram(this);
  // Li afegim els shaders corresponents
  program->addShader(&fs);
  program->addShader(&vs);
  // Linkem el program
  program->link();
  // Indiquem que aquest és el program que volem usar
  program->bind();

  // Obtenim identificador per a l'atribut “vertex” del vertex shader
  vertexLoc = glGetAttribLocation (program->programId(), "vertex");
  // Obtenim identificador per a l'atribut “normal” del vertex shader
  normalLoc = glGetAttribLocation (program->programId(), "normal");
  // Obtenim identificador per a l'atribut “matamb” del vertex shader
  matambLoc = glGetAttribLocation (program->programId(), "matamb");
  // Obtenim identificador per a l'atribut “matdiff” del vertex shader
  matdiffLoc = glGetAttribLocation (program->programId(), "matdiff");
  // Obtenim identificador per a l'atribut “matspec” del vertex shader
  matspecLoc = glGetAttribLocation (program->programId(), "matspec");
  // Obtenim identificador per a l'atribut “matshin” del vertex shader
  matshinLoc = glGetAttribLocation (program->programId(), "matshin");

  // Demanem identificadors per als uniforms dels shaders
  transLoc = glGetUniformLocation (program->programId(), "TG");
  projLoc = glGetUniformLocation (program->programId(), "proj");
  viewLoc = glGetUniformLocation (program->programId(), "view");
  colorLoc = glGetUniformLocation (program->programId(), "colorMul");
}


