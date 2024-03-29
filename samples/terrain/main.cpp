#include <vector>
#include <string>
#include <memory>
#include "oglw.h"

template <class T>
using uptr = std::unique_ptr<T>;
using namespace OGLW;

// ------------------------------------------------------------------------------
// OGLW App
class TestApp : public App {
    public:
        TestApp() : App("OGLW::TestApp", /*"Roboto-Regular.ttf", */1024, 720) {}
        void update(float _dt) override;
        void render(float _dt) override;
        void init() override;
        void captureReflectionTexture(float _yWaterPlane, glm::mat4 _model);
        void drawTerrain(glm::mat4 _model);
        void drawWater(glm::mat4 _model, float _yWaterPlane);

    private:
        uptr<Shader> m_shader;
        uptr<Shader> m_waterShader;
        uptr<Mesh<glm::vec4>> m_geometry;
        uptr<Mesh<glm::vec4>> m_waterGeometry;
        uptr<Texture> m_texture;
        uptr<RenderTarget> m_reflectionRenderTarget;
        uptr<RenderTarget> m_depthRenderTarget;
        uptr<Camera> m_reflectionCamera;
        uptr<QuadRenderer> m_quadRenderer;
};
OGLWMain(TestApp);

void TestApp::init() {

    // default camera
    m_camera.setPosition({0.0, -3.0, 5.0});
    m_camera.setFar(30.f);
    m_camera.setNear(0.5f);
    m_camera.setFov(30);

    m_reflectionCamera = uptr<Camera>(new Camera());
    m_reflectionCamera->setFar(200.0f);
    m_reflectionCamera->setNear(0.1f);
    m_reflectionCamera->setFov(30);

    m_shader = uptr<Shader>(new Shader("default.glsl"));
    m_waterShader = uptr<Shader>(new Shader("water.glsl"));

    OGLW::TextureOptions options = {
        GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
        {GL_LINEAR, GL_LINEAR},
        {GL_REPEAT, GL_REPEAT}
    };

    m_texture = uptr<OGLW::Texture>(new Texture("perlin.png", options));

    m_geometry = plane(20.f, 20.f, 350, 350);
    m_waterGeometry = plane(20.f, 20.f, 150, 150);

    RenderTargetSetup setup;
    setup.useDepth = true;
    m_reflectionRenderTarget = std::make_unique<OGLW::RenderTarget>(setup);
    m_reflectionRenderTarget->create(1024, 720);

    RenderTargetSetup depthSetup;
    depthSetup.useDepthTexture = true;
    m_depthRenderTarget = std::make_unique<OGLW::RenderTarget>(depthSetup);
    m_depthRenderTarget->create(1024, 720);

    m_quadRenderer = uptr<QuadRenderer>(new QuadRenderer());
    m_quadRenderer->init();
}

void TestApp::update(float _dt) {

    oglwUpdateFreeFlyCamera(_dt, 'S', 'W', 'A', 'D', 1e-3f, 20.f);

    //oglwDisplayText(24.f, {20.f, 40.f}, "X:" + std::to_string(theta) + " Y: " + std::to_string(phi), true);
}

void TestApp::captureReflectionTexture(float _yWaterPlane, glm::mat4 _model) {
    glm::mat4 mvp;

    m_texture->bind(0);

    glEnable(GL_CLIP_PLANE0);

    glm::vec3 camPosition = m_camera.getPosition();
    camPosition.y *= -1.0f;
    camPosition.y -= 2.0 * _yWaterPlane;

    m_reflectionCamera->setPosition(camPosition);
    m_reflectionCamera->setRotationX(-m_camera.getRotation().x);
    m_reflectionCamera->setRotationY(m_camera.getRotation().y);

    mvp = m_reflectionCamera->getProjectionMatrix() * m_reflectionCamera->getViewMatrix() * _model;

    m_shader->setUniform("mvp", mvp);
    m_shader->setUniform("tex", 0);
    m_shader->setUniform("clipPlane", glm::vec4(0.0, 0.0, 1.0, -_yWaterPlane));
    m_shader->setUniform("modelView", m_reflectionCamera->getViewMatrix() * _model);
    m_shader->setUniform("normalMatrix", glm::inverse(glm::transpose(glm::mat3(mvp))));
    m_shader->setUniform("lightPosition", glm::vec3(0.0, 0.0, -5.0));

    RenderState::depthTest(GL_TRUE);
    RenderState::culling(GL_TRUE);
    RenderState::cullFace(GL_BACK);
    RenderState::blending(GL_FALSE);

    m_reflectionRenderTarget->apply(1024, 720, 0xffffffff);

    m_geometry->draw(*m_shader);

    glDisable(GL_CLIP_PLANE0);

    RenderTarget::applyDefault(1024, 720, 0xffffffff);
}

void TestApp::drawTerrain(glm::mat4 _model) {

    glm::mat4 mvp = m_camera.getProjectionMatrix() * m_camera.getViewMatrix() * _model;

    m_texture->bind(0);

    m_shader->setUniform("mvp", mvp);
    m_shader->setUniform("modelView", m_camera.getViewMatrix() * _model);
    m_shader->setUniform("normalMatrix", glm::inverse(glm::transpose(glm::mat3(mvp))));

    RenderState::depthTest(GL_TRUE);
    RenderState::culling(GL_TRUE);
    RenderState::cullFace(GL_BACK);
    RenderState::blending(GL_FALSE);

    m_geometry->draw(*m_shader);

}

void TestApp::drawWater(glm::mat4 _model, float _yWaterPlane) {

    glm::mat4 mvp = m_camera.getProjectionMatrix() * m_camera.getViewMatrix() * _model;

    m_reflectionRenderTarget->getRenderTexture()->bind(0);
    m_depthRenderTarget->getDepthRenderTexture()->bind(1);

    m_waterShader->setUniform("mvp", mvp);
    m_waterShader->setUniform("time", m_globalTime);
    m_waterShader->setUniform("modelView", m_camera.getViewMatrix() * _model);
    m_waterShader->setUniform("yWaterPlane", _yWaterPlane);
    m_waterShader->setUniform("normalMatrix", glm::inverse(glm::transpose(glm::mat3(mvp))));
    m_waterShader->setUniform("screenResolution", getResolution());
    m_waterShader->setUniform("reflectionTexture", 0);
    m_waterShader->setUniform("depthMap", 1);
    m_waterShader->setUniform("near", m_camera.getNear());
    m_waterShader->setUniform("far", m_camera.getFar());
    m_waterShader->setUniform("lightPosition", glm::vec3(0.0, 0.0, -5.0));

    RenderState::blending(GL_TRUE);
    RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_waterGeometry->draw(*m_waterShader);

}


void TestApp::render(float _dt) {
    float yWaterPlane = 2.0f;
    glm::mat4 model = glm::rotate(glm::mat4(), (float) M_PI_2, glm::vec3(1.0, 0.0, 0.0));

    captureReflectionTexture(yWaterPlane, model);

    /// Draw terrain

    m_depthRenderTarget->apply(1024, 720, 0xffffffff);

    drawTerrain(model);

    RenderTarget::applyDefault(1024, 720, 0xffffffff);

    drawTerrain(model);

    /// Draw water

    drawWater(model, yWaterPlane);

    /// Debug draw camera framebuffer

    m_quadRenderer->render(*m_reflectionRenderTarget->getRenderTexture(), getResolution(), glm::vec2(0.0, 0.0), 256);
    m_quadRenderer->render(*m_depthRenderTarget->getDepthRenderTexture(), getResolution(), glm::vec2(0.0, 256), 256);
}

