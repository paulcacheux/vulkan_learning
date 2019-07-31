#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "context.hpp"
#include "game.hpp"
#include "scene.hpp"
#include "swapchain.hpp"
#include "vk_mem_alloc.h"

namespace app {
class Window;
}

namespace vulkan {

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {
    struct SyncObject {
        VkSemaphore imageAvailable;
        VkSemaphore renderFinished;
        VkFence inFlight;
    };

  public:
    Renderer(const app::Window& appWindow, Context& context);
    ~Renderer();

    bool checkValidationLayerSupport();
    void drawFrame();
    void recreateSwapchain();
    void setMustRecreateSwapchain() {
        _mustRecreateSwapchain = true;
    }
    void updateUniformBuffer(uint32_t currentImage);

    void setScene(const scene::Scene* scene);
    void setViewMatrix(glm::mat4 viewMatrix);

    std::unique_ptr<BufferManager> bufferManager;
    Context& context;

  private:
    std::vector<SyncObject> _createSyncObjects();

    const scene::Scene* _scene;
    glm::mat4 _viewMatrix;
    const app::Window& _appWindow;
    std::unique_ptr<Swapchain> _swapchain;
    std::vector<SyncObject> _syncObjects;
    std::size_t currentFrame = 0;
    bool _mustRecreateSwapchain = false;

    friend struct Swapchain;
};

} // namespace vulkan

#endif
