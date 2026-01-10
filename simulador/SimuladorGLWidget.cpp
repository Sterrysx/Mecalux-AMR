#include "SimuladorGLWidget.h"

#include <iostream>
#include <QImage>
#include <QPainter>

// Helper function to generate industrial floor texture
QImage generateIndustrialFloor(int size = 512) {
    QImage image(size, size, QImage::Format_RGB888);
    image.fill(QColor(40, 40, 45)); // Dark base color

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw the grid lines (Tile gaps)
    QPen pen(QColor(25, 25, 30)); // Darker lines (grout)
    pen.setWidth(4);
    painter.setPen(pen);
    
    int tileSize = 64; // Size of one tile
    
    for (int i = 0; i <= size; i += tileSize) {
        painter.drawLine(i, 0, i, size); // Vertical
        painter.drawLine(0, i, size, i); // Horizontal
    }

    // Add some "noise" to make it look like concrete (simple speckles)
    for (int i = 0; i < 5000; ++i) {
        int x = rand() % size;
        int y = rand() % size;
        int val = (rand() % 20) - 10; // Slight variation
        QColor pixel = image.pixelColor(x, y);
        
        int r = qBound(0, pixel.red() + val, 255);
        int g = qBound(0, pixel.green() + val, 255);
        int b = qBound(0, pixel.blue() + val, 255);
        
        image.setPixelColor(x, y, QColor(r, g, b));
    }

    return image;
}

SimuladorGLWidget::SimuladorGLWidget (QWidget* parent) : QOpenGLWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);  // per rebre events de teclat
  xClick = yClick = 0;
  DoingInteractive = NONE;
  animationTimer = new QTimer(this);
  connect(animationTimer, &QTimer::timeout, this, &SimuladorGLWidget::updateAnimation);
  //animationTimer->start(16);
  robotCamera = false;  // Initialize to false - start with normal camera
  staticObjectsDirty = true;  // Initially need to draw static objects
  monitoringBackend_ = false;
  
  // Initialize telemetry reader
  telemetryReader_ = new TelemetryReader(this);
  connect(telemetryReader_, &TelemetryReader::robotsUpdated, 
          this, &SimuladorGLWidget::updateRobotsFromBackend);
}

SimuladorGLWidget::~SimuladorGLWidget ()
{
  if (program != NULL)
    delete program;
}


void SimuladorGLWidget::initializeGL ()
{
  // Initialize robots map with tuple (x, y, direction, hasBox)
  robots = std::map<int,std::tuple<float,float,float,bool>>();
  
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();  

  glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_STENCIL_TEST);  // Enable stencil testing

  carregaShaders();
  iniEscena ();
  iniCamera ();
  
  // Load floor texture from image file
  std::cout << "Loading floor texture from image...\n";
  QImage floorImage("./textures/warehouse_floor.jpg");
  
  if (floorImage.isNull()) {
    std::cout << "ERROR: Could not load floor image!\n";
    floorImage = generateIndustrialFloor(512);
  } else {
    std::cout << "Successfully loaded floor texture image (" 
              << floorImage.width() << "x" << floorImage.height() << ")\n";
  }
  
  // Convert to OpenGL format
  QImage glFormattedImage = floorImage.convertToFormat(QImage::Format_RGBA8888).mirrored();
  
  // Generate and bind texture
  glGenTextures(1, &floorTextureID);
  glBindTexture(GL_TEXTURE_2D, floorTextureID);
  
  // Upload texture data
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glFormattedImage.width(), glFormattedImage.height(),
               0, GL_RGBA, GL_UNSIGNED_BYTE, glFormattedImage.bits());
  
  // Set texture parameters - clamp to edge to avoid repeating
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  // Generate mipmaps
  glGenerateMipmap(GL_TEXTURE_2D);
  
  std::cout << "Floor texture ready (" << glFormattedImage.width() << "x" << glFormattedImage.height() << ")\n";
  
  // Load warehouse layout
  loadWarehouse("warehouse_layout.json");
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
      projectTransform();
    }
    update();
  std::cout << "Robot seleccionat amb ID: " << selectedRObotID << "\n";
}

void SimuladorGLWidget::updateAnimation() {
  makeCurrent();
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
  zoomFactor = 1.0f;  // Initialize zoom to 1.0 (no zoom)
  floorScale = glm::vec3(1.0f, 1.0f, 1.0f);  // Initialize floor scale to 1.0 (no scaling)
  orto = false;
  projectTransform();
  viewTransform();
}

void SimuladorGLWidget::loadWarehouse(const QString& filename) {
    makeCurrent();
    if (warehouseLoader.loadFromJSON(filename)) {
        // Load robots from warehouse (replace existing robots)
        robots = warehouseLoader.getRobots();
        
        // Update nextRobotID based on highest ID in loaded robots
        nextRobotID = 1;
        for (const auto& robot : robots) {
            emit robotAfegit(robot.first );
            if (robot.first >= nextRobotID) {
                nextRobotID = robot.first + 1;
            }
        }
        
        // Update floor size
        glm::vec2 floorSize = warehouseLoader.getFloorSize();
        updateFloorSize(floorSize.x, floorSize.y);
        
        // Mark static objects as dirty (need to be redrawn)
        staticObjectsDirty = true;
        
        std::cout << "Warehouse loaded successfully with " 
                  << warehouseLoader.getObjects().size() << " objects and "
                  << robots.size() << " robots\n";
        update();
    } else {
        qWarning() << "Failed to load warehouse from" << filename;
    }
}

void SimuladorGLWidget::afegirRobot(int x, int y) {
    makeCurrent();
    std::cout << "Afegir robot a la posició: (" << x << ", " << y << ")\n";
    robots[nextRobotID] = std::make_tuple(float(x), float(y), 0.f, false);  // Start without box
    emit robotAfegit(nextRobotID);
    setRobotBoxState(nextRobotID, true);  // Initialize hasBox to false
    nextRobotID++;
    update();
}

void SimuladorGLWidget::setRobotBoxState(int robotID, bool hasBox) {
    makeCurrent();
    auto it = robots.find(robotID);
    if (it != robots.end()) {
        float x = std::get<0>(it->second);
        float y = std::get<1>(it->second);
        float angle = std::get<2>(it->second);
        robots[robotID] = std::make_tuple(x, y, angle, hasBox);
        std::cout << "Robot " << robotID << " box state set to: " << (hasBox ? "true" : "false") << "\n";
        update();
    } else {
        std::cerr << "Robot " << robotID << " not found\n";
    }
}

void SimuladorGLWidget::eliminarRobot(int robotID){
  makeCurrent();
  robots.erase(robotID);
  std::cout << "Eliminar robot amb ID: " << robotID << "\n";
  update();
}

void SimuladorGLWidget::modelTransforRobot (int id, float x, float y,float angle){
    glm::mat4 TG(1.0f);
    
    // Position translation
    TG = glm::translate(TG, glm::vec3(x, 0.f, y));
    // Rotation based on direction
    float angleRad = glm::radians(angle); // 90 degrees per direction
    TG = glm::rotate(TG, angleRad, glm::vec3(0, 1, 0));
    //glm::vec3 escalaModel = glm::vec3(1);
    //TG= glm::scale(TG, escalaModel);  // Scale down robot size
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

  // Draw static objects (floor and warehouse objects) only when necessary
  paintStaticObjects();

  // Reset color multiplier to white (neutral) for proper material rendering
  glUniform3fv(colorLoc, 1, &white[0]);

  // First pass: Draw all non-selected robots normally
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0x00);
  for (const auto& robot : robots) {
        if (robot.first == selectedRObotID) continue;
        
        const float x = std::get<0>(robot.second);
        const float y = std::get<1>(robot.second);
        const int dir = std::get<2>(robot.second);
        const bool hasBox = std::get<3>(robot.second);

        // Select model based on whether robot is carrying a box
        const int modelIndex = hasBox ? 2 : 1;  // Model 2 = with box, Model 1 = without box

        // Ensure color multiplier is white for proper material colors
        glUniform3fv(colorLoc, 1, &white[0]);
        
        modelTransforRobot(robot.first, x, y, dir);
        glBindVertexArray(VAO_models[modelIndex]);
        glDrawArrays(GL_TRIANGLES, 0, models[modelIndex].faces().size() * 3);
    }

    // If we have a selected robot, draw it with outline
    if (selectedRObotID != -1) {
        auto it = robots.find(selectedRObotID);
        if (it != robots.end()) {
            const float x = std::get<0>(it->second);
            const float y = std::get<1>(it->second);
            const float angle = std::get<2>(it->second);
            const bool hasBox = std::get<3>(it->second);

            // Select model based on whether robot is carrying a box
            const int modelIndex = hasBox ? 2 : 1;  // Model 2 = with box, Model 1 = without box

            // First render: Write to stencil buffer
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            
            modelTransforRobot(selectedRObotID, x, y, angle);
            glBindVertexArray(VAO_models[modelIndex]);
            glDrawArrays(GL_TRIANGLES, 0, models[modelIndex].faces().size() * 3);

            // Second render: Draw slightly scaled outline
            if(!robotCamera){
              glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
              glStencilMask(0x00);
              glDisable(GL_DEPTH_TEST);

              // Create scaled transform for outline (use the same model index)
              glm::vec3 transDesp = glm::vec3(0, centreCapsaModels[modelIndex].y - minY[modelIndex], 0);
              glm::mat4 TG(1.0f);
              TG = glm::translate(TG, glm::vec3(x, 0.f, y));
              float angleRad = glm::radians(angle);
              TG = glm::rotate(TG, angleRad, glm::vec3(0,1,0));
              TG = glm::translate(TG, transDesp);
              TG = glm::scale(TG, glm::vec3(1.1f)); // Scale up for outline
              TG = glm::translate(TG, glm::vec3(-centreCapsaModels[modelIndex].x, -centreCapsaModels[modelIndex].y, -centreCapsaModels[modelIndex].z));
              
              // Draw outline in red
              glm::vec3 outlineColor(1.0f, 0.0f, 0.0f); // Red outline
              glUniform3fv(colorLoc, 1, &outlineColor[0]);
              glUniformMatrix4fv(transLoc, 1, GL_FALSE, &TG[0][0]);
              glDrawArrays(GL_TRIANGLES, 0, models[modelIndex].faces().size() * 3);
            }
            // Restore states
            glEnable(GL_DEPTH_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glUniform3fv(colorLoc, 1, &white[0]);
        }
    }
    
  glBindVertexArray(0);
}

void SimuladorGLWidget::paintStaticObjects()
{
    // Switch to floor shader program
    floorProgram->bind();
    
    // Set uniforms for floor shader (projection and view)
    glUniformMatrix4fv(floorProjLoc, 1, GL_FALSE, &Proj[0][0]);  
    glUniformMatrix4fv(floorViewLoc, 1, GL_FALSE, &View[0][0]);
    
    // Draw ground with texture
    glStencilMask(0x00);
    
    // Bind floor texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTextureID);
    glUniform1i(floorTextureLoc, 0);
    
    glBindVertexArray(VAO_Terra);
    modelTransformGround();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    // Switch back to main shader program for other objects
    program->bind();
    
    // Draw warehouse static objects from JSON
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0x00);
    glUniform3fv(colorLoc, 1, &white[0]);
    
    const std::vector<WarehouseObject>& warehouseObjects = warehouseLoader.getObjects();
    for (const auto& obj : warehouseObjects) {
        if (obj.modelIndex < 0 || obj.modelIndex >= NUM_MODELS) {
            continue; // Skip invalid model indices
        }
        
        // Create transformation matrix with JSON dimensions for scaling
        glm::mat4 TG(1.0f);
        
        // 1. Translate to object center position
        TG = glm::translate(TG, obj.center);
        
        // 2. Rotate around Y axis
        TG = glm::rotate(TG, glm::radians(-obj.rotation), glm::vec3(0, 1, 0));
        
        // 3. Scale model to match JSON dimensions
        // obj.dimensions contains [width, height, depth] from JSON
        glm::vec3 scaleFactors = obj.dimensions / dimensions[obj.modelIndex];
        TG = glm::scale(TG, scaleFactors);
        
        // 4. Center the model at origin (after scaling)
        TG = glm::translate(TG, glm::vec3(-centreCapsaModels[obj.modelIndex].x, 
                                          -minY[obj.modelIndex], 
                                          -centreCapsaModels[obj.modelIndex].z));
        
        glUniformMatrix4fv(transLoc, 1, GL_FALSE, &TG[0][0]);
        glBindVertexArray(VAO_models[obj.modelIndex]);
        glDrawArrays(GL_TRIANGLES, 0, models[obj.modelIndex].faces().size() * 3);
    }
    
    // Debug: Draw picking zones as black points on the ground
    const std::vector<PickingZone>& zones = warehouseLoader.getPickingZones();
    if (!zones.empty()) {
        glm::vec3 blackColor(0.0f, 0.0f, 0.0f); // Black color for debug points
        glUniform3fv(colorLoc, 1, &blackColor[0]);
        
        for (const auto& zone : zones) {
            glm::mat4 TG(1.0f);
            
            // Position at zone center (slightly above ground to avoid z-fighting)
            glm::vec3 position = zone.center;
            position.y = 0.05f; // Slightly above ground
            TG = glm::translate(TG, position);
            
            // Small scale for debug point (0.3 units diameter)
            TG = glm::scale(TG, glm::vec3(0.3f, 0.3f, 0.3f));
            
            // Use model 1 (robot model) as debug marker
            int debugModelIndex = 1;
            TG = glm::translate(TG, glm::vec3(-centreCapsaModels[debugModelIndex].x, 
                                              -minY[debugModelIndex], 
                                              -centreCapsaModels[debugModelIndex].z));
            
            glUniformMatrix4fv(transLoc, 1, GL_FALSE, &TG[0][0]);
            glBindVertexArray(VAO_models[debugModelIndex]);
            glDrawArrays(GL_TRIANGLES, 0, models[debugModelIndex].faces().size() * 3);
        }
    }
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
  TG = glm::scale(TG, floorScale);
  // Use floor shader transform location (floorTransLoc is only valid when floor shader is bound)
  glUniformMatrix4fv (floorTransLoc, 1, GL_FALSE, &TG[0][0]);
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
    // Apply zoom by dividing FOV (smaller FOV = more zoom)
    Proj = glm::perspective(fov / zoomFactor, ra, zn, zf);
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
    // Apply zoom to orthographic projection
    left /= zoomFactor;
    right /= zoomFactor;
    top /= zoomFactor;
    botom /= zoomFactor;
    
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
            const float angle = std::get<2>(it->second);
            
            // Calculate forward direction based on robot rotation
            float angleRad = glm::radians(angle);
            glm::vec3 forward(sin(angleRad), 0, cos(angleRad));
            // Calculate robot dimensions
            float robotHeight = centreCapsaModels[1].y - minY[1];
            float robotDepth = centreCapsaModels[1].z - minZ[1];  // Half-depth from center to front edge
            
            // Position camera at the front of the robot, centered and at eye height
            glm::vec3 cameraPos(x, robotHeight, y);
            cameraPos += robotDepth * forward;  // Move to front edge based on robot's facing direction
            
            // Look point is ahead of robot in its facing direction
            glm::vec3 lookAt = cameraPos + forward * 3.f;
            
            View = glm::lookAt(
                cameraPos,          
                lookAt,             
                glm::vec3(0,1,0)    
            );

            // Since camera is at the front edge, near plane should be small but enough
            // to prevent seeing through the floor when looking down
            zn = 0.01f;  // Very small near plane since we're at the front edge
            zf = 50.0f;  // Far enough to see the entire floor (floor is 30x30)
        }
    } else if (!orto) {
        // Regular perspective camera
        float angleeX_2 = angleY + glm::radians(180.0f);
        View = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -2*radiEsc));
        View = glm::rotate(View, angleX, glm::vec3(1, 0, 0));
        View = glm::rotate(View, -angleeX_2, glm::vec3(0, 1, 0));
        View = glm::translate(View, -centreEsc);
        zf=3*radiEsc;
    } else {
        // Orthographic top-down view
        zn=0.1f;
        zf=3*radiEsc;
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
      robotCamera = false;
      zoomFactor = 1.0f;  // Reset zoom when changing camera mode
      staticObjectsDirty = true;  // Mark static objects to be redrawn
      viewTransform();
      projectTransform();
      update();
      break;
    }
    case Qt::Key_Z: {
        if (selectedRObotID != -1) {

            robotCamera = !robotCamera;
            orto = false;
            zoomFactor = 1.0f;  // Reset zoom when changing camera mode
            staticObjectsDirty = true;  // Mark static objects to be redrawn
            viewTransform();
            projectTransform();
            update();
        }
        break;
    }
    case Qt::Key_L: {
        // Reset zoom
        zoomFactor = 1.0f;
        projectTransform();
        update();
        break;
    }
    case Qt::Key_P: {
        if (selectedRObotID != -1) {

            auto r =robots.find(selectedRObotID);
            if(r != robots.end()) {
                float x = std::get<0>(r->second);
                float y = std::get<1>(r->second);
                float angle = std::get<2>(r->second);
                bool hasBox = std::get<3>(r->second);
                
                // Move robot forward by 0.5 units in its facing direction

                if(angle==0.0f){
                    angle=90.0f;
                }else angle=0.0f;
                // Update robot position (preserve hasBox state)
                robots[selectedRObotID] = std::make_tuple(x, y, angle, hasBox);
            }
            viewTransform();
            projectTransform();
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

void SimuladorGLWidget::wheelEvent(QWheelEvent *event)
{
  // Disable zoom in robot camera mode
  if (robotCamera) {
    return;
  }
  
  makeCurrent();
  // Get the scroll delta (positive = zoom in, negative = zoom out)
  int delta = event->angleDelta().y();
  
  if (delta > 0) {
    // Zoom in (increase zoom factor)
    zoomFactor *= 1.1f;
  } else if (delta < 0) {
    // Zoom out (decrease zoom factor)
    zoomFactor /= 1.1f;
  }
  
  // Clamp zoom factor to reasonable values
  if (zoomFactor < 0.1f) zoomFactor = 0.1f;
  if (zoomFactor > 10.0f) zoomFactor = 10.0f;
  
  projectTransform();
  update();
}

void SimuladorGLWidget::calculaCapsaModel (Model &p, float &escala, float ampladaDesitjada, glm::vec3 &centreCapsa,float &minY,float &minX,float &minZ,glm::vec3 &dimensions)
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
  dimensions = glm::vec3(maxx-minx, maxy-miny, maxz-minz);
  std::cout << dimensions.x << " " << dimensions.y << " " << dimensions.z << std::endl;
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
    std::cout<< "Calculant capsa model " << objNames[i] << std::endl;
    calculaCapsaModel (models[i], escalaModels[i], ampladesDesitjades[i], centreCapsaModels[i],minY[i],minX[i],minZ[i], dimensions[i]);
  
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
  amb = glm::vec3(0.2,0.2,0.2);  // Gris fosc per l'ambient
  diff = glm::vec3(0.4,0.4,0.4);  // Gris per la difusa
  spec = glm::vec3(0.0,0.0,0.0);  // Sense reflexió especular (terra mat)
  shin = 10;  // Brillantor mínima
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

  // VBO amb les coordenades de textura
  glm::vec2 texterra[] = {
	glm::vec2(0.0, 1.0),  // Use texture once without repeating
	glm::vec2(1.0, 1.0),
	glm::vec2(0.0, 0.0),
	glm::vec2(1.0, 0.0),
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

  GLuint VBO_Terra[7];  // Increased from 6 to 7 for texture coordinates
  glGenBuffers(7, VBO_Terra);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(posterra), posterra, GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc (floor shader)
  glVertexAttribPointer(floorVertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorVertexLoc);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normterra), normterra, GL_STATIC_DRAW);

  // Activem l'atribut normalLoc (floor shader)
  glVertexAttribPointer(floorNormalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorNormalLoc);

  // Buffer de coordenades de textura
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(texterra), texterra, GL_STATIC_DRAW);

  glVertexAttribPointer(floorTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorTexCoordLoc);

  // Buffer de component ambient
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matamb), matamb, GL_STATIC_DRAW);

  glVertexAttribPointer(floorMatambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorMatambLoc);

  // Buffer de component difusa
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[4]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matdiff), matdiff, GL_STATIC_DRAW);

  glVertexAttribPointer(floorMatdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorMatdiffLoc);

  // Buffer de component especular
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[5]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matspec), matspec, GL_STATIC_DRAW);

  glVertexAttribPointer(floorMatspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorMatspecLoc);

  // Buffer de component shininness
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Terra[6]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matshin), matshin, GL_STATIC_DRAW);

  glVertexAttribPointer(floorMatshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(floorMatshinLoc);

  glBindVertexArray(0);
}

void SimuladorGLWidget::updateFloorSize(float width, float depth) {
  // The default floor is 31x31 units (from -1 to 30 in both X and Z)
  // Calculate scale factors based on desired size
  float defaultWidth = 31.0f;
  float defaultDepth = 31.0f;
  
  floorScale.x = width / defaultWidth;
  floorScale.z = depth / defaultDepth;
  floorScale.y = 1.0f;  // Keep Y scale at 1.0 (don't scale vertically)
  
  // Mark static objects as dirty (floor changed)
  staticObjectsDirty = true;
  
  std::cout << "Floor size updated to: " << width << " x " << depth 
            << " (scale: " << floorScale.x << ", " << floorScale.z << ")" << std::endl;
}


void SimuladorGLWidget::carregaShaders() 
{
  // Creem els shaders per al fragment shader i el vertex shader
  QOpenGLShader fs (QOpenGLShader::Fragment, this);
  QOpenGLShader vs (QOpenGLShader::Vertex, this);
  // Carreguem el codi dels fitxers i els compilem
  if (!fs.compileSourceFile("./shaders/basicLlumShader.frag")) {
    std::cout << "ERROR: Fragment shader compilation failed:\n" << fs.log().toStdString() << std::endl;
  }
  if (!vs.compileSourceFile("./shaders/basicLlumShader.vert")) {
    std::cout << "ERROR: Vertex shader compilation failed:\n" << vs.log().toStdString() << std::endl;
  }
  // Creem el program
  program = new QOpenGLShaderProgram(this);
  // Li afegim els shaders corresponents
  program->addShader(&fs);
  program->addShader(&vs);
  // Linkem el program
  if (!program->link()) {
    std::cout << "ERROR: Shader program linking failed:\n" << program->log().toStdString() << std::endl;
  }
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
  
  // Create separate shader program for textured floor
  QOpenGLShader floorFs (QOpenGLShader::Fragment, this);
  QOpenGLShader floorVs (QOpenGLShader::Vertex, this);
  floorFs.compileSourceFile("./shaders/texturedFloor.frag");
  floorVs.compileSourceFile("./shaders/texturedFloor.vert");
  
  floorProgram = new QOpenGLShaderProgram(this);
  floorProgram->addShader(&floorFs);
  floorProgram->addShader(&floorVs);
  floorProgram->link();
  floorProgram->bind();
  
  // Get attribute locations for floor shader
  floorVertexLoc = glGetAttribLocation (floorProgram->programId(), "vertex");
  floorNormalLoc = glGetAttribLocation (floorProgram->programId(), "normal");
  floorTexCoordLoc = glGetAttribLocation (floorProgram->programId(), "texCoord");
  floorMatambLoc = glGetAttribLocation (floorProgram->programId(), "matamb");
  floorMatdiffLoc = glGetAttribLocation (floorProgram->programId(), "matdiff");
  floorMatspecLoc = glGetAttribLocation (floorProgram->programId(), "matspec");
  floorMatshinLoc = glGetAttribLocation (floorProgram->programId(), "matshin");
  
  // Get uniform locations for floor shader
  floorTransLoc = glGetUniformLocation (floorProgram->programId(), "TG");
  floorProjLoc = glGetUniformLocation (floorProgram->programId(), "proj");
  floorViewLoc = glGetUniformLocation (floorProgram->programId(), "view");
  floorTextureLoc = glGetUniformLocation (floorProgram->programId(), "floorTexture");
  
  // Switch back to main program
  program->bind();
}



// =============================================================================
// BACKEND INTEGRATION METHODS
// =============================================================================

void SimuladorGLWidget::startBackendMonitoring(const QString& telemetryDir)
{
    qDebug() << "Starting backend monitoring from:" << telemetryDir;
    telemetryReader_->startMonitoring(telemetryDir);
    monitoringBackend_ = true;
}

void SimuladorGLWidget::stopBackendMonitoring()
{
    qDebug() << "Stopping backend monitoring";
    telemetryReader_->stopMonitoring();
    monitoringBackend_ = false;
}

bool SimuladorGLWidget::isMonitoringBackend() const
{
    return monitoringBackend_;
}

void SimuladorGLWidget::updateRobotsFromBackend(const std::map<int, TelemetryReader::RobotData>& robotData)
{
    makeCurrent();
    
    bool selectedRobotUpdated = false;
    
    // Update robot positions from backend data
    for (const auto& pair : robotData) {
        int robotId = pair.first;
        const TelemetryReader::RobotData& data = pair.second;
        
        // Convert coordinates from backend scale to simulator scale (divide by 10)
        float x = data.x / 10.0f;
        float y = data.y / 10.0f;
        
        // Calculate angle from velocity vector
        float angle = 0.0f;
        if (data.vx != 0.0f || data.vy != 0.0f) {
            // atan2 gives angle in radians, convert to degrees
            angle = atan2(data.vx, data.vy) * 180.0f / M_PI;
        } else if (robots.find(robotId) != robots.end()) {
            // If not moving, keep the previous angle
            angle = std::get<2>(robots[robotId]);
        }
        
        if(robots.find(robotId) == robots.end()) {
          emit robotAfegit(robotId);
        }
        
        // Update robot tuple (x, y, angle, hasPackage)
        robots[robotId] = std::make_tuple(x, y, angle, data.hasPackage);
        
        // Track if we updated the selected robot
        if (robotId == selectedRObotID) {
            selectedRobotUpdated = true;
        }
    }
    
    // If we're in robot camera mode and the selected robot was updated,
    // update the view and projection transforms
    if (robotCamera && selectedRobotUpdated && selectedRObotID != -1) {
        viewTransform();
        projectTransform();
    }
    
    update();
}
