#ifndef fsMain_H_
#define fsMain_H_

#include "gameplay.h"

using namespace gameplay;

class fsMain;

class fsAutoBindingResolver : RenderState::AutoBindingResolver
{
    public:
    fsMain * _fsmain;
    fsAutoBindingResolver(fsMain* fsmain);
    bool resolveAutoBinding(const char* autoBinding, Node* node, MaterialParameter* parameter);
};


/**
 * Main game class.
 */
class fsMain : public Game
{
public:

    /**
     * Constructor.
     */
    fsMain();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
  Vector3 getLightDirection0() const;
   Vector3 getLightColor0() const;	

protected:

    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

	void enableScriptCamera(bool enable);

	void setScriptCameraSpeed(float normal, float fast);

	bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

private:

    /**
     * Draws the scene each frame.
     */
    bool drawScene(Node* node);
    fsAutoBindingResolver* _resolver;
    Scene* _scene;
    bool _wireframe;

	Font* _font;
	Terrain* _terrain;
	Node* _sky;
	Light* _directionalLight;
	Node* _sphere;
	Node* _box;




};

#endif

