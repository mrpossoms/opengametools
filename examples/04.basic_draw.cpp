#include <g.h>

const std::string vs_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"void main (void) {"
"gl_Position = vec4(a_position, 1.0);"
"}";

const std::string fs_src =
"void main (void) {"
"gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);"
"}";


struct my_core : public g::core
{
    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
                                               .add_src<GL_FRAGMENT_SHADER>(fs_src)
                                               .create();

        return true;
    }

    virtual void update(float dt)
    {

    }

    g::gfx::shader basic_shader;
};


int main (int argc, const char* argv[])
{
    my_core core;

    core.start({});

    return 0;
}
