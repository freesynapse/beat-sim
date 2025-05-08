
#include <synapse/Synapse>
#include <synapse/SynapseAddons/Figure/Figure.h>
#include <synapse/SynapseMain.hpp>

#include "SerialReader.hpp"
#include "filter.h"

using namespace Syn;

//
class layer : public Layer
{
public:
    layer() : Layer("layer") {}
    virtual ~layer() {}

    virtual void onAttach() override;
    virtual void onUpdate(float _dt) override;
    void onResize(Event *_e);
    virtual void onImGuiRender() override;
    
    void onKeyDownEvent(Event *_e);
    void onMouseButtonEvent(Event *_e);

private:
    Ref<Framebuffer> m_renderBuffer = nullptr;
    Ref<Font> m_font = nullptr;
    glm::ivec2 m_vp = { 0, 0 };

    Ref<SerialReader<float>> m_reader = nullptr;

    float m_signalFreq = 60.0f;
    Ref<mplc::Figure> m_abpPlot = nullptr;
    Ref<mplc::Figure> m_filteredPlot = nullptr;
    Ref<mplc::Figure> m_ssfPlot = nullptr;
    Ref<Filter> m_filter = nullptr;

    // flags
    bool m_wireframeMode = false;
    bool m_toggleCulling = false;
    bool m_serialIsPaused = false;

};

//
class syn_app_instance : public Application
{
public:
    syn_app_instance() { this->pushLayer(new layer); }
};
Application* CreateSynapseApplication() { return new syn_app_instance(); }

//---------------------------------------------------------------------------------------
void layer::onAttach()
{
    // register event callbacks
    EventHandler::register_callback(EventType::INPUT_KEY, SYN_EVENT_MEMBER_FNC(layer::onKeyDownEvent));
    EventHandler::register_callback(EventType::INPUT_MOUSE_BUTTON, SYN_EVENT_MEMBER_FNC(layer::onMouseButtonEvent));
    EventHandler::register_callback(EventType::VIEWPORT_RESIZE, SYN_EVENT_MEMBER_FNC(layer::onResize));
    EventHandler::push_event(new WindowToggleFullscreenEvent());

    m_renderBuffer = API::newFramebuffer(ColorFormat::RGBA16F, glm::ivec2(0), 1, true, true, "render_buffer");

    // load font
    m_font = API::newFont("../assets/ttf/JetBrains/JetBrainsMono-Medium.ttf", 14.0f);
    m_font->setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // reading from serial port
    m_reader = MakeRef<SerialReader<float>>(__global_argc, __global_argv);
    if (Error::error())
       EventHandler::push_event(new WindowCloseEvent());
    m_reader->array().set(0.0f);

    // ABP plot
    static glm::vec2 fig_sz = { 2000, 300 };
    m_abpPlot = MakeRef<mplc::Figure>(fig_sz);
    mplc::lineplot_params_t params;
    params.x_nice_scale = false;
    params.line_width_px = 2.0f;
    params.line_color = glm::vec4(0.9f, 0.3f, 0.3f, 1.0f);
    
    // for beat markers
    mplc::scatter_params_t scatter_params;
    scatter_params.marker_color = glm::vec4(1.0f);
    scatter_params.marker = mplc::FigureMarker::VLine;
    scatter_params.marker_size = 7.0f;
    
    std::vector<float> vdata(BUFFER_SIZE);
    m_abpPlot->lineplot(vdata, "ABP", params);
    // m_abpPlot->lineplot({0}, {0}, "ABP_beats", params);
    m_abpPlot->scatter({0}, {0}, "ABP_beats", scatter_params);
    m_abpPlot->title("ABP");
    m_abpPlot->render();

    // BW LP filter
    m_filter = MakeRef<Filter>(BUFFER_SIZE, (size_t)m_signalFreq);

    // filtered plot
    m_filteredPlot = MakeRef<mplc::Figure>(fig_sz);
    params.line_color = glm::vec4(0.26f, 0.53f, 0.96f, 1.0f);
    m_filteredPlot->lineplot(std::vector<float>(m_filter->y(), m_filter->y() + m_filter->n()), 
                            "filtered", 
                            params);
    m_filteredPlot->title("ABP_f");

    // SSF plot
    m_ssfPlot = MakeRef<mplc::Figure>(fig_sz);
    m_ssfPlot->lineplot(std::vector<float>(m_filter->z(), m_filter->z() + m_filter->n()), 
                       "SSF", 
                       params);
    // m_ssfPlot->scatter({0}, {0}, "SSF_beats", scatter_params);
    m_ssfPlot->title("SSF(ABP_f)");

    // general settings
	Renderer::get().setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	Renderer::get().disableImGuiUpdateReport();

    Application::get().setMaxFPS(60.0f);
    
}

//---------------------------------------------------------------------------------------
void layer::onUpdate(float _dt)
{
    SYN_PROFILE_FUNCTION();
	
    static auto& renderer = Renderer::get();

    m_renderBuffer->bind();
    renderer.setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_wireframeMode)
        renderer.enableWireFrame();    

    // -- BEGINNING OF SCENE -- //

    // -- END OF SCENE -- //


    if (m_wireframeMode)
        renderer.disableWireFrame();

	
    // Text rendering 
    // TODO: all text rendering should go into an overlay layer.
    static float fontHeight = m_font->getFontHeight() + 1.0f;
    int i = 0;
    m_font->beginRenderBlock();
	m_font->addString(2.0f, fontHeight * ++i, "fps=%.0f  VSYNC=%s", TimeStep::getFPS(), Application::get().getWindow().isVSYNCenabled() ? "ON" : "OFF");
    // if (m_filter->timeSec() > 3.0f)
    m_font->addString(2.0f, fontHeight * ++i, "mean SSF: %.1f", m_filter->meanSSF());
    m_font->endRenderBlock();

    //
    m_renderBuffer->bindDefaultFramebuffer();

    // read from serial port
    if (!m_serialIsPaused)
    {
        m_reader->read<float>();
        if (m_reader->updated())
        {
            // render ABP (finish plot after beat detection)
            std::vector<float> vdata(m_reader->array().data, m_reader->array().data + m_reader->dataSize());
            m_abpPlot->canvas("ABP")->data(vdata);

            // filter data
            m_filter->applyLPFilter(&m_reader->array());
            std::vector<float> fdata(m_filter->y(), m_filter->y() + m_filter->n());
            m_filteredPlot->data(fdata);
            m_filteredPlot->setYLim({ -10.0f, 200.0f });
            m_filteredPlot->render();

            // compute SSF and plot
            m_filter->computeSSF(5);
            std::vector<float> ssf_data(m_filter->z(), m_filter->z() + m_filter->n());
            m_ssfPlot->canvas("SSF")->data(ssf_data);

            // detect beats if stable signal
            if (m_filter->timeSec() > 3.0f)
            {
                m_filter->detectBeats();
                if (m_filter->beatsDetected())
                {
                    auto &beats = m_filter->beatOnsets();
                    std::vector<float> x;
                    std::vector<float> y_abp;
                    for (size_t i = 0; i < beats.size() - 1; i++)
                    {
                        float beat = (float)beats[i];
                        x.push_back(Syn::max(beat-2, 0.0f));
                        y_abp.push_back(100.0f);
                    }
                    m_abpPlot->canvas("ABP_beats")->data(x, y_abp);
                }
            }

            m_ssfPlot->setYLim({-10.0f, 120.0f});
            m_ssfPlot->render();

            // finish ABP plot
            m_abpPlot->setYLim({-50.0f, 200.0f});
            m_abpPlot->render();

        }

        //
        m_reader->reset();

    }

}
 
//---------------------------------------------------------------------------------------
void layer::onResize(Event *_e)
{
    ViewportResizeEvent *e = dynamic_cast<ViewportResizeEvent*>(_e);
    m_vp = e->getViewport();

}
//---------------------------------------------------------------------------------------
void layer::onKeyDownEvent(Event *_e)
{
    KeyDownEvent *e = dynamic_cast<KeyDownEvent*>(_e);
    static bool vsync = true;

    if (e->getAction() == SYN_KEY_PRESSED)
    {
        switch (e->getKey())
        {
            case SYN_KEY_Z: vsync = !vsync; Application::get().getWindow().setVSYNC(vsync); break;
            case SYN_KEY_V:         m_renderBuffer->saveAsPNG(); break;
            case SYN_KEY_ESCAPE:    EventHandler::push_event(new WindowCloseEvent()); break;
            case SYN_KEY_F4:        m_wireframeMode = !m_wireframeMode; break;
            case SYN_KEY_F5:        m_toggleCulling = !m_toggleCulling; Renderer::setCulling(m_toggleCulling); break;
            //
            case SYN_KEY_SPACE:
                m_serialIsPaused = !m_serialIsPaused;
                //if (m_filter->beatsDetected())
                //{
                //    auto &beats = m_filter->beatOnsets();
                //    printf("beats: ");
                //    for (auto &beat : beats)
                //        printf("%d ", beat);
                //    printf("\n");
                //}
                break;
            default: break;
        }
    }
    
}

//---------------------------------------------------------------------------------------
void layer::onMouseButtonEvent(Event *_e)
{
    MouseButtonEvent *e = dynamic_cast<MouseButtonEvent *>(_e);

    switch (e->getButton())
    {
        case SYN_MOUSE_BUTTON_1:    break;
        case SYN_MOUSE_BUTTON_2:    break;
        default: break;
    }
    
}

//---------------------------------------------------------------------------------------
void layer::onImGuiRender()
{
    static bool p_open = true;

    static bool opt_fullscreen_persistant = true;
    static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
    bool opt_fullscreen = opt_fullscreen_persistant;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
    	ImGuiViewport* viewport = ImGui::GetMainViewport();
    	ImGui::SetNextWindowPos(viewport->Pos);
    	ImGui::SetNextWindowSize(viewport->Size);
    	ImGui::SetNextWindowViewport(viewport->ID);
    	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (opt_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	    window_flags |= ImGuiWindowFlags_NoBackground;

    window_flags |= ImGuiWindowFlags_NoTitleBar;

    ImGui::GetCurrentContext()->NavWindowingToggleLayer = false;

    //-----------------------------------------------------------------------------------
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("synapse-core", &p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
	    ImGui::PopStyleVar(2);

    // Dockspace
    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = 0;
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        dockspace_id = ImGui::GetID("dockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
    }
	
    //-----------------------------------------------------------------------------------
    // set the 'rest' of the window as the viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("synapse-core::renderer");
    static ImVec2 oldSize = { 0, 0 };
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x != oldSize.x || viewportSize.y != oldSize.y)
    {
        // dispatch a viewport resize event -- registered classes will receive this.
        EventHandler::push_event(new ViewportResizeEvent(glm::vec2(viewportSize.x, viewportSize.y)));
        SYN_CORE_TRACE("viewport [ ", viewportSize.x, ", ", viewportSize.y, " ]");
        oldSize = viewportSize;
    }

    // direct ImGui to the framebuffer texture
    ImGui::Image((void*)m_renderBuffer->getColorAttachmentIDn(0), viewportSize, { 0, 1 }, { 1, 0 });

    ImGui::End();
    ImGui::PopStyleVar();

    // ABP
    ImGui::Begin("serial read");
    if (m_abpPlot != nullptr)
       ImGui::Image((void*)m_abpPlot->framebufferTexturePtr(), m_abpPlot->size(), { 0, 1 }, { 1, 0 });
    ImGui::End();

    // filtered ABP
    ImGui::Begin("filter");
    if (m_filteredPlot != nullptr)
       ImGui::Image((void*)m_filteredPlot->framebufferTexturePtr(), m_filteredPlot->size(), { 0, 1 }, { 1, 0 });
    ImGui::End();

    // SSF
    ImGui::Begin("SSF");
    if (m_ssfPlot != nullptr)
       ImGui::Image((void*)m_ssfPlot->framebufferTexturePtr(), m_ssfPlot->size(), { 0, 1 }, { 1, 0 });
    ImGui::End();


    // end root
    ImGui::End();

}
