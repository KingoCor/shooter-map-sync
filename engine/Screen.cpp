//
// Created by Иван Ильин on 14.01.2021.
//

#include "Screen.h"
#include "utils/Time.h"
#include <utility>
#include "utils/Log.h"
#include "ResourceManager.h"
#include <SFML/OpenGL.hpp>
#include <cmath>


void Screen::open(int screenWidth, int screenHeight, const std::string &name, bool verticalSync, sf::Color background, sf::Uint32 style) {
    _title = name;
    _w = screenWidth;
    _h = screenHeight;
    _background = background;

    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.antialiasingLevel = 8;

    _window->create(sf::VideoMode(_w, _h), name, style, settings);
    _window->setVerticalSyncEnabled(verticalSync);
}

void Screen::display() {
    sf::Event event{};
    while (_window->pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            _window->close();
        }
    }

    std::string title = _title + " (" + std::to_string(Time::fps()) + " fps)";
    _window->setTitle(title);

    _window->display();
}

void Screen::clear() {
    // Clear the depth buffer
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    _window->clear(_background);
}

void Screen::drawTriangle(const Triangle& triangle)
{
    sf::Vertex tris[3] =
            {
                    sf::Vertex(sf::Vector2f((float)triangle[0].x(), (float)triangle[0].y()), triangle.color()),
                    sf::Vertex(sf::Vector2f((float)triangle[1].x(), (float)triangle[1].y()), triangle.color()),
                    sf::Vertex(sf::Vector2f((float)triangle[2].x(), (float)triangle[2].y()), triangle.color())
            };

    _window->pushGLStates();
    _window->draw(tris, 3, sf::Triangles);
    _window->popGLStates();
}

void Screen::setTitle(const std::string& title) {
    _title = title;
}

bool Screen::isOpen() {
    return _window->isOpen();
}

void Screen::close() {
    _window->close();
}

void Screen::setMouseCursorVisible(bool visible) {
    _window->setMouseCursorVisible(visible);
}

void Screen::drawTetragon(const Vec2D &p1, const Vec2D &p2, const Vec2D &p3, const Vec2D &p4, sf::Color color) {
    sf::ConvexShape polygon;
    polygon.setPointCount(4);
    polygon.setPoint(0, sf::Vector2f((float)p1.x(), (float)p1.y()));
    polygon.setPoint(1, sf::Vector2f((float)p2.x(), (float)p2.y()));
    polygon.setPoint(2, sf::Vector2f((float)p3.x(), (float)p3.y()));
    polygon.setPoint(3, sf::Vector2f((float)p4.x(), (float)p4.y()));
    polygon.setFillColor(color);

    _window->pushGLStates();
    _window->draw(polygon);
    _window->popGLStates();
}

void Screen::drawText(const std::string& string, const Vec2D &position, int size, sf::Color color) {
    sf::Text text;

    text.setFont(*ResourceManager::loadFont(Consts::MEDIUM_FONT));

    text.setCharacterSize(size);
    text.setFillColor(color);
    text.setStyle(sf::Text::Italic);
    text.setPosition((float)position.x(), (float)position.y());

    text.setString(string);

    _window->pushGLStates();
    _window->draw(text);
    _window->popGLStates();
}

void Screen::drawSprite(const sf::Sprite &sprite) {
    _window->pushGLStates();
    _window->draw(sprite);
    _window->popGLStates();
}

void Screen::drawText(const sf::Text &text) {
    _window->pushGLStates();
    _window->draw(text);
    _window->popGLStates();
}

// OpenGL functions
void Screen::glDrawMesh(GLfloat* geometry, GLfloat* view, GLfloat* model, size_t count) {
    // OpenGL:
    // Make the window the active window for OpenGL calls
    _window->setActive(true);

    glEnable(GL_CULL_FACE); // enable culling face
    glCullFace(GL_BACK); // cull faces from back
    glFrontFace(GL_CCW); // vertex order (counter clock wise)

    // Enable Z-buffer read and write
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearDepth(1.f);

    // Disable lighting
    glDisable(GL_LIGHTING);

    // enable alpha channel:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Configure the viewport (the same size as the window)
    glViewport(0, 0, _window->getSize().x, _window->getSize().y);

    // Setup a perspective projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat ratio = static_cast<float>(_window->getSize().x) / _window->getSize().y;
    glFrustum(-ratio, ratio, -1.f, 1.f, 1.0f, 500.f);

    // Enable position and texture coordinates vertex components
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 7 * sizeof(GLfloat), geometry);
    glColorPointer(4, GL_FLOAT, 7 * sizeof(GLfloat), geometry + 3);

    // Disable normal and color vertex components
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    // Apply some transformations
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLoadMatrixf(view);
    glMultMatrixf(model);

    // Draw the mesh
    glDrawArrays(GL_TRIANGLES, 0, count);

    // Make the window no longer the active window for OpenGL calls
    _window->setActive(false);
}

GLfloat* Screen::glMeshToGLfloatArray(std::shared_ptr<Mesh> mesh, const Vec3D& cameraPosition) {
    std::vector<Triangle>& triangles = mesh->triangles();

    auto* geometry = (GLfloat*)malloc(7*3*triangles.size()*sizeof(GLfloat));

    for(int i = 0; i < triangles.size(); i++) {

        int stride = 21*i;

        double dot = triangles[i].norm().dot((mesh->model()*Vec3D(triangles[i][0]) - cameraPosition).normalized());
        sf::Color color = triangles[i].color();
        sf::Color ambientColor = sf::Color((sf::Uint8)(color.r * (0.3 * std::abs(dot) + 0.7)),
                                           (sf::Uint8)(color.g * (0.3 * std::abs(dot) + 0.7)),
                                           (sf::Uint8)(color.b * (0.3 * std::abs(dot) + 0.7)),
                                           (sf::Uint8)color.a);


        geometry[stride + 0] = (GLfloat)triangles[i][0].x();
        geometry[stride + 1] = (GLfloat)triangles[i][0].y();
        geometry[stride + 2] = (GLfloat)triangles[i][0].z();

        geometry[stride + 3] = (GLfloat)ambientColor.r/255.0f;
        geometry[stride + 4] = (GLfloat)ambientColor.g/255.0f;
        geometry[stride + 5] = (GLfloat)ambientColor.b/255.0f;
        geometry[stride + 6] = (GLfloat)ambientColor.a/255.0f;

        geometry[stride + 7] = (GLfloat)triangles[i][1].x();
        geometry[stride + 8] = (GLfloat)triangles[i][1].y();
        geometry[stride + 9] = (GLfloat)triangles[i][1].z();

        geometry[stride + 10] = (GLfloat)ambientColor.r/255.0f;
        geometry[stride + 11] = (GLfloat)ambientColor.g/255.0f;
        geometry[stride + 12] = (GLfloat)ambientColor.b/255.0f;
        geometry[stride + 13] = (GLfloat)ambientColor.a/255.0f;

        geometry[stride + 14] = (GLfloat)triangles[i][2].x();
        geometry[stride + 15] = (GLfloat)triangles[i][2].y();
        geometry[stride + 16] = (GLfloat)triangles[i][2].z();

        geometry[stride + 17] = (GLfloat)ambientColor.r/255.0f;
        geometry[stride + 18] = (GLfloat)ambientColor.g/255.0f;
        geometry[stride + 19] = (GLfloat)ambientColor.b/255.0f;
        geometry[stride + 20] = (GLfloat)ambientColor.a/255.0f;
    }
    return geometry;
}
