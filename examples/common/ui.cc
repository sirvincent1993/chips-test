//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "ui.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_time.h"
#include "imgui.h"

typedef struct {
    ImVec2 disp_size;
} vs_params_t;

static uint64_t last_time;
static bool btn_down[SAPP_MAX_MOUSEBUTTONS];
static bool btn_up[SAPP_MAX_MOUSEBUTTONS];
static const int MaxVertices = (1<<17);
static const int MaxIndices = MaxVertices * 3;
static sg_pipeline pip;
static sg_bindings bind;

static void ui_draw_imgui(ImDrawData*);
static void ui_init_imgui(void);

extern const char* vs_src_imgui;
extern const char* fs_src_imgui;

static ui_draw_t ui_draw_cb;

void ui_init(ui_draw_t draw_cb) {
    ui_draw_cb = draw_cb;
    ui_init_imgui();
}

void ui_discard(void) {
    // FIXME
}

void ui_draw(void) {
    const int cur_width = sapp_width();
    const int cur_height = sapp_height();
    uint64_t frame_time = stm_laptime(&last_time);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(cur_width), float(cur_height));
    io.DeltaTime = (float) stm_sec(frame_time);
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++) {
        if (btn_down[i]) {
            btn_down[i] = false;
            io.MouseDown[i] = true;
        }
        else if (btn_up[i]) {
            btn_up[i] = false;
            io.MouseDown[i] = false;
        }
    }
    if (io.WantTextInput && !sapp_keyboard_shown()) {
        sapp_show_keyboard(true);
    }
    if (!io.WantTextInput && sapp_keyboard_shown()) {
        sapp_show_keyboard(false);
    }
    ImGui::NewFrame();
    if (ui_draw_cb) {
        ui_draw_cb();
    }
    ImGui::Render();
    ui_draw_imgui(ImGui::GetDrawData());
}

bool ui_input(const sapp_event* event) {
    auto& io = ImGui::GetIO();
    io.KeyAlt = (event->modifiers & SAPP_MODIFIER_ALT) != 0;
    io.KeyCtrl = (event->modifiers & SAPP_MODIFIER_CTRL) != 0;
    io.KeyShift = (event->modifiers & SAPP_MODIFIER_SHIFT) != 0;
    io.KeySuper = (event->modifiers & SAPP_MODIFIER_SUPER) != 0;
    switch (event->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            io.MousePos.x = event->mouse_x;
            io.MousePos.y = event->mouse_y;
            btn_down[event->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            io.MousePos.x = event->mouse_x;
            io.MousePos.y = event->mouse_y;
            btn_up[event->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            io.MousePos.x = event->mouse_x;
            io.MousePos.y = event->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_ENTER:
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            for (int i = 0; i < 3; i++) {
                btn_down[i] = false;
                btn_up[i] = false;
                io.MouseDown[i] = false;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            io.MouseWheelH = event->scroll_x;
            io.MouseWheel = event->scroll_y;
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            btn_down[0] = true;
            io.MousePos.x = event->touches[0].pos_x;
            io.MousePos.y = event->touches[0].pos_y;
            break;
        case SAPP_EVENTTYPE_TOUCHES_MOVED:
            io.MousePos.x = event->touches[0].pos_x;
            io.MousePos.y = event->touches[0].pos_y;
            break;
        case SAPP_EVENTTYPE_TOUCHES_ENDED:
            btn_up[0] = true;
            io.MousePos.x = event->touches[0].pos_x;
            io.MousePos.y = event->touches[0].pos_y;
            break;
        case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
            btn_up[0] = btn_down[0] = false;
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
            io.KeysDown[event->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            io.KeysDown[event->key_code] = false;
            break;
        case SAPP_EVENTTYPE_CHAR:
            io.AddInputCharacter((ImWchar)event->char_code);
            break;
        default:
            break;
    }
    return io.WantCaptureKeyboard;
}

void ui_init_imgui(void) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.IniFilename = nullptr;
    io.KeyMap[ImGuiKey_Tab] = SAPP_KEYCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SAPP_KEYCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SAPP_KEYCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SAPP_KEYCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SAPP_KEYCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SAPP_KEYCODE_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = SAPP_KEYCODE_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = SAPP_KEYCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SAPP_KEYCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SAPP_KEYCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SAPP_KEYCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SAPP_KEYCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SAPP_KEYCODE_ENTER;
    io.KeyMap[ImGuiKey_Escape] = SAPP_KEYCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SAPP_KEYCODE_A;
    io.KeyMap[ImGuiKey_C] = SAPP_KEYCODE_C;
    io.KeyMap[ImGuiKey_V] = SAPP_KEYCODE_V;
    io.KeyMap[ImGuiKey_X] = SAPP_KEYCODE_X;
    io.KeyMap[ImGuiKey_Y] = SAPP_KEYCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SAPP_KEYCODE_Z;

    // dynamic vertex- and index-buffers for imgui-generated geometry
    sg_buffer_desc vbuf_desc = { };
    vbuf_desc.usage = SG_USAGE_STREAM;
    vbuf_desc.size = MaxVertices * sizeof(ImDrawVert);
    bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    sg_buffer_desc ibuf_desc = { };
    ibuf_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    ibuf_desc.usage = SG_USAGE_STREAM;
    ibuf_desc.size = MaxIndices * sizeof(ImDrawIdx);
    bind.index_buffer = sg_make_buffer(&ibuf_desc);

    // font texture for imgui's default font
    unsigned char* font_pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
    sg_image_desc img_desc = { };
    img_desc.width = font_width;
    img_desc.height = font_height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    img_desc.min_filter = SG_FILTER_LINEAR;
    img_desc.mag_filter = SG_FILTER_LINEAR;
    img_desc.content.subimage[0][0].ptr = font_pixels;
    img_desc.content.subimage[0][0].size = font_width * font_height * 4;
    io.Fonts->TexID = (ImTextureID)(uintptr_t) sg_make_image(&img_desc).id;

    // shader object for imgui rendering
    sg_shader_desc shd_desc = { };
    auto& ub = shd_desc.vs.uniform_blocks[0];
    ub.size = sizeof(vs_params_t);
    ub.uniforms[0].name = "disp_size";
    ub.uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
    shd_desc.fs.images[0].name = "tex";
    shd_desc.fs.images[0].type = SG_IMAGETYPE_2D;
    shd_desc.vs.source = vs_src_imgui;
    shd_desc.fs.source = fs_src_imgui;
    sg_shader shd = sg_make_shader(&shd_desc);

    // pipeline object for imgui rendering
    sg_pipeline_desc pip_desc = { };
    pip_desc.layout.buffers[0].stride = sizeof(ImDrawVert);
    auto& attrs = pip_desc.layout.attrs;
    attrs[0].name="position"; attrs[0].sem_name="POSITION"; attrs[0].offset=offsetof(ImDrawVert, pos); attrs[0].format=SG_VERTEXFORMAT_FLOAT2;
    attrs[1].name="texcoord0"; attrs[1].sem_name="TEXCOORD"; attrs[1].offset=offsetof(ImDrawVert, uv); attrs[1].format=SG_VERTEXFORMAT_FLOAT2;
    attrs[2].name="color0"; attrs[2].sem_name="COLOR"; attrs[2].offset=offsetof(ImDrawVert, col); attrs[2].format=SG_VERTEXFORMAT_UBYTE4N;
    pip_desc.shader = shd;
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    pip_desc.blend.enabled = true;
    pip_desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.color_write_mask = SG_COLORMASK_RGB;
    pip = sg_make_pipeline(&pip_desc);
}

// draw ImGui draw lists via sokol-gfx
void ui_draw_imgui(ImDrawData* draw_data) {
    assert(draw_data);
    if (draw_data->CmdListsCount == 0) {
        return;
    }

    // render the command list
    vs_params_t vs_params;
    vs_params.disp_size.x = ImGui::GetIO().DisplaySize.x;
    vs_params.disp_size.y = ImGui::GetIO().DisplaySize.y;
    for (int cl_index = 0; cl_index < draw_data->CmdListsCount; cl_index++) {
        const ImDrawList* cl = draw_data->CmdLists[cl_index];

        // append vertices and indices to buffers, record start offsets in draw state
        const int vtx_size = cl->VtxBuffer.size() * sizeof(ImDrawVert);
        const int idx_size = cl->IdxBuffer.size() * sizeof(ImDrawIdx);
        const int vb_offset = sg_append_buffer(bind.vertex_buffers[0], &cl->VtxBuffer.front(), vtx_size);
        const int ib_offset = sg_append_buffer(bind.index_buffer, &cl->IdxBuffer.front(), idx_size);
        /* don't render anything if the buffer is in overflow state (this is also
            checked internally in sokol_gfx, draw calls that attempt from
            overflowed buffers will be silently dropped)
        */
        if (sg_query_buffer_overflow(bind.vertex_buffers[0]) ||
            sg_query_buffer_overflow(bind.index_buffer))
        {
            continue;
        }

        ImTextureID tex_id = ImGui::GetIO().Fonts->TexID;
        bind.vertex_buffer_offsets[0] = vb_offset;
        bind.index_buffer_offset = ib_offset;
        bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;

        sg_apply_pipeline(pip);
        sg_apply_bindings(&bind);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
        int base_element = 0;
        for (const ImDrawCmd& pcmd : cl->CmdBuffer) {
            if (pcmd.UserCallback) {
                pcmd.UserCallback(cl, &pcmd);
            }
            else {
                if (tex_id != pcmd.TextureId) {
                    tex_id = pcmd.TextureId;
                    bind.fs_images[0].id = (uint32_t)(uintptr_t)tex_id;
                    sg_apply_bindings(&bind);
                }
                const int scissor_x = (int) (pcmd.ClipRect.x);
                const int scissor_y = (int) (pcmd.ClipRect.y);
                const int scissor_w = (int) (pcmd.ClipRect.z - pcmd.ClipRect.x);
                const int scissor_h = (int) (pcmd.ClipRect.w - pcmd.ClipRect.y);
                sg_apply_scissor_rect(scissor_x, scissor_y, scissor_w, scissor_h, true);
                sg_draw(base_element, pcmd.ElemCount, 1);
            }
            base_element += pcmd.ElemCount;
        }
    }
}

#if defined(SOKOL_GLCORE33)
const char* vs_src_imgui =
    "#version 330\n"
    "uniform vec2 disp_size;\n"
    "in vec2 position;\n"
    "in vec2 texcoord0;\n"
    "in vec4 color0;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
    "    uv = texcoord0;\n"
    "    color = color0;\n"
    "}\n";
const char* fs_src_imgui =
    "#version 330\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = texture(tex, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
const char* vs_src_imgui =
    "uniform vec2 disp_size;\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord0;\n"
    "attribute vec4 color0;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
    "    uv = texcoord0;\n"
    "    color = color0;\n"
    "}\n";
const char* fs_src_imgui =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex, uv) * color;\n"
    "}\n";
#elif defined(SOKOL_METAL)
const char* vs_src_imgui =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct params_t {\n"
    "  float2 disp_size;\n"
    "};\n"
    "struct vs_in {\n"
    "  float2 pos [[attribute(0)]];\n"
    "  float2 uv [[attribute(1)]];\n"
    "  float4 color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "  float4 pos [[position]];\n"
    "  float2 uv;\n"
    "  float4 color;\n"
    "};\n"
    "vertex vs_out _main(vs_in in [[stage_in]], constant params_t& params [[buffer(0)]]) {\n"
    "  vs_out out;\n"
    "  out.pos = float4(((in.pos / params.disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
    "  out.uv = in.uv;\n"
    "  out.color = in.color;\n"
    "  return out;\n"
    "}\n";
const char* fs_src_imgui =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "  float2 uv;\n"
    "  float4 color;\n"
    "};\n"
    "fragment float4 _main(fs_in in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
    "  return tex.sample(smp, in.uv) * in.color;\n"
    "}\n";
#elif defined(SOKOL_D3D11)
const char* vs_src_imgui =
    "cbuffer params {\n"
    "  float2 disp_size;\n"
    "};\n"
    "struct vs_in {\n"
    "  float2 pos: POSITION;\n"
    "  float2 uv: TEXCOORD0;\n"
    "  float4 color: COLOR0;\n"
    "};\n"
    "struct vs_out {\n"
    "  float2 uv: TEXCOORD0;\n"
    "  float4 color: COLOR0;\n"
    "  float4 pos: SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "  vs_out outp;\n"
    "  outp.pos = float4(((inp.pos/disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
    "  outp.uv = inp.uv;\n"
    "  outp.color = inp.color;\n"
    "  return outp;\n"
    "}\n";
const char* fs_src_imgui =
    "Texture2D<float4> tex: register(t0);\n"
    "sampler smp: register(s0);\n"
    "float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0): SV_Target0 {\n"
    "  return tex.Sample(smp, uv) * color;\n"
    "}\n";
#endif
