/**
 @file GWindow.cpp
  
 @maintainer Morgan McGuire, matrix@graphics3d.com
 
 @created 2004-11-16
 @edited  2006-01-16
 */

#include "GLG3D/GWindow.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

void GWindow::loadExtensions() {
    GLCaps::init();
}

}
