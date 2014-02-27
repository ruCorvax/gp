#include "MeshGame.h"

// Declare our game instance
fsMain game;

fsMain::fsMain()
:  _font(NULL), _terrain(NULL), _sky(NULL), _scene(NULL), _wireframe(false), _directionalLight(NULL), _resolver(NULL)
{
}

void fsMain::initialize()
{
    // Load game scene from file
    //_scene = Scene::load("res/demo.scene");
    _resolver = new fsAutoBindingResolver(this);
	_scene = Scene::load("res/common/terrain/sample.scene");
	_terrain = _scene->findNode("terrain")->getTerrain();
	_sky = _scene->findNode("sky");
	_sky->setTag("lighting", "none");

	_font = Font::create("res/ui/arial.gpb");
	// Load shapes
	Bundle* bundle;


    // Set the aspect ratio for the scene's camera to match the current resolution
    _scene->getActiveCamera()->setAspectRatio(getAspectRatio());

	// Use script camera for navigation
	// Load camera script
	getScriptController()->loadScript("res/common/camera.lua");

	enableScriptCamera(true);
	setScriptCameraSpeed(20, 80);

	_directionalLight = _scene->findNode("directionalLight")->getLight();

}

void fsMain::finalize()
{
    SAFE_RELEASE(_font);
    SAFE_RELEASE(_scene);
    delete _resolver;	
}

void fsMain::update(float elapsedTime)
{
    // Rotate model
	getScriptController()->executeFunction<void>("camera_update", "f", elapsedTime);
        Node* camera = _scene->getActiveCamera()->getNode();
	Vector3 pos = camera->getTranslationWorld();
	_terrain->update(pos.x, pos.z);
	float height = _terrain->getHeight(pos.x, pos.z);
	camera->setTranslationY(height + 10);

}

void fsMain::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);

    // Visit all the nodes in the scene for drawing
    _scene->visit(this, &fsMain::drawScene);

    char buffer[1024];
    sprintf(buffer, "FPS: %d", getFrameRate());
    _font->start();
    _font->drawText(buffer, 65, 18, Vector4::one(), 30);

    _font->finish();

}


void fsMain::keyEvent(Keyboard::KeyEvent evt, int key)
{
	getScriptController()->executeFunction<void>("camera_keyEvent", "[Keyboard::KeyEvent][Keyboard::Key]", evt, key);

    if (evt == Keyboard::KEY_PRESS)
    {
        switch (key)
        {
        case Keyboard::KEY_ESCAPE:
            exit();
            break;
        }
    }
}

bool fsMain::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
	return false;
}


void fsMain::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{

	getScriptController()->executeFunction<void>("camera_touchEvent", "[Touch::TouchEvent]iiui", evt, x, y, contactIndex);

	switch (evt)
    {
    case Touch::TOUCH_PRESS:
        //_wireframe = !_wireframe;
		getScriptController()->executeFunction<void>("camera_touchEvent", "[Touch::TouchEvent]iiui", evt, x, y, contactIndex);
        break;
    case Touch::TOUCH_RELEASE:
        break;
    case Touch::TOUCH_MOVE:
        break;
    };
}

void fsMain::enableScriptCamera(bool enable)
{
	Game::getInstance()->getScriptController()->executeFunction<void>("camera_setActive", "b", enable);
}

void fsMain::setScriptCameraSpeed(float normal, float fast)
{
	Game::getInstance()->getScriptController()->executeFunction<void>("camera_setSpeed", "ff", normal, fast);
}

bool fsMain::drawScene(Node* node)
{
	Terrain* terrain;

	Model* model = node->getModel();
	if (model)
	{
		model->draw(_wireframe);
	}

/*	if (node->getModel())
	{
		if (node->getBoundingSphere().intersects(camera->getFrustum()))
		{
			node->getModel()->draw();
		}
	}
	else */ if ((terrain = node->getTerrain()))
	{
		terrain->draw(_wireframe);
	}

	return true;
}


Vector3 fsMain::getLightDirection0() const
{
	return _directionalLight->getNode()->getForwardVectorView();
}

Vector3 fsMain::getLightColor0() const
{
	return _directionalLight->getColor();
}

fsAutoBindingResolver::fsAutoBindingResolver(fsMain* fsmain) {
    _fsmain = fsmain;
}

bool fsAutoBindingResolver::resolveAutoBinding(const char* autoBinding, Node* node, MaterialParameter* parameter)
{
   if (strcmp(autoBinding, "LIGHT_DIRECTION_0") == 0)
   {
      parameter->bindValue(this->_fsmain, &fsMain::getLightDirection0);
      return true;
   }
   else if (strcmp(autoBinding, "LIGHT_COLOR_0") == 0)
   {
      parameter->bindValue(this->_fsmain, &fsMain::getLightColor0);
      return true;
   }

   return false;
}


