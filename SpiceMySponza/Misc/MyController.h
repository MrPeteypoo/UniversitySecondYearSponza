#pragma once
#include <tygra/WindowControlDelegate.hpp>
#include <SceneModel/SceneModel_fwd.hpp>

class MyView;

class MyController : public tygra::WindowControlDelegate
{
public:
    
    MyController();

    ~MyController();

private:

    void
    windowControlWillStart(std::shared_ptr<tygra::Window> window) override;

    void
    windowControlDidStop(std::shared_ptr<tygra::Window> window) override;

    void
    windowControlViewWillRender(std::shared_ptr<tygra::Window> window) override;

    void
    windowControlMouseMoved(std::shared_ptr<tygra::Window> window,
                            int x,
                            int y) override;	

    void
    windowControlMouseButtonChanged(std::shared_ptr<tygra::Window> window,
                                    int button_index,
                                    bool down) override;

    void
    windowControlMouseWheelMoved(std::shared_ptr<tygra::Window> window,
                                 int position) override;

    void
    windowControlKeyboardChanged(std::shared_ptr<tygra::Window> window,
                                 int key_index,
                                 bool down) override;

    void
    windowControlGamepadAxisMoved(std::shared_ptr<tygra::Window> window,
                                  int gamepad_index,
                                  int axis_index,
                                  float pos) override;

    void
    windowControlGamepadButtonChanged(std::shared_ptr<tygra::Window> window,
                                      int gamepad_index,
                                      int button_index,
                                      bool down) override;

    void
    updateCameraTranslation();

    std::shared_ptr<MyView> view_;
    std::shared_ptr<SceneModel::Context> scene_;

    bool camera_turn_mode_;
	float camera_move_speed_[4];
	float camera_rotate_speed_[2];
};
