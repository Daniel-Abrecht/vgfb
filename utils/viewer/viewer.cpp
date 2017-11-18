/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This program is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <errno.h>
#include <linux/fb.h>
#include <vg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <QApplication>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <viewer.hpp>

FBViewer::FBViewer(QWidget *parent)
 : QMainWindow(parent)
{
  var = new struct fb_var_screeninfo();
  fb = -1;
  setWindowTitle("FB Viewer");
  label = new QLabel(this);
  scroll_area = new QScrollArea(this);
  scroll_area->setWidget(label);
  setCentralWidget(scroll_area);
  timer = new QTimer(this);
  timer->setInterval(1000/60);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start();
}

void FBViewer::resizeEvent(QResizeEvent* event){
  QMainWindow::resizeEvent(event);
  QSize ws = this->size();
  QSize ls = label->size();
  if( ls.width() > ws.width() || ls.height() > ws.height() ){
    scroll_area->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    scroll_area->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  }else{
    scroll_area->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scroll_area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  }
  if( var->xres != (unsigned long)ws.width() || var->yres != (unsigned long)ws.height() ){
    struct fb_var_screeninfo v = *var;
    v.xres = ws.width();
    v.yres = ws.height();
    ioctl(fb, FBIOPUT_VSCREENINFO, &v);
  }
  update();
}

void FBViewer::update(){
  if(!checkChanges())
    return;
  if(!memory_size)
    return;
  size_t offset = var->yoffset * var->xres_virtual * 4;
  label->setPixmap(QPixmap::fromImage(QImage(memory+offset,var->xres,var->yres,(QImage::Format)format)));
}

bool FBViewer::checkChanges(){
  struct fb_var_screeninfo var;
  if( ioctl(fb, FBIOGET_VSCREENINFO, &var) == -1 ){
    std::cerr << "FBIOGET_VSCREENINFO failed: " << strerror(errno) << std::endl;
    return false;
  }
  format = QImage::Format_RGBX8888; // TODO
  std::size_t size = var.xres_virtual * var.yres_virtual * 4;
  if( size != memory_size ){
    if( memory && memory != MAP_FAILED )
      munmap(memory,memory_size);
    memory_size = 0;
    while( ioctl(fb, VGFB_WAIT_RESIZE_DONE) == -1 && errno == EINTR );
    memory = (unsigned char*)mmap(0, size, PROT_READ, MAP_SHARED, fb, 0);
    if( !memory || memory==MAP_FAILED ){
      std::cerr << "mmap failed: " << strerror(errno) << std::endl;
      return false;
    }
    memory_size = size;
  }
  if( var.xres != this->var->xres || var.yres != this->var->yres ){
    resize(var.xres,var.yres);
    label->resize(var.xres,var.yres);
  }
  *this->var = var;
  return true;
}

bool FBViewer::setFB(const char* path){
  if(fb)
    ::close(fb);
  fb = ::open(path,O_RDWR);
  if( fb == -1 ){
    std::cerr << "open failed: " << strerror(errno) << std::endl;
    return false;
  }
  {
    int fb_minor;
    if( ioctl(fb, VGFBM_GET_FB_MINOR, &fb_minor) != -1 )
      std::cout << "Other framebuffer is /dev/fb" << fb_minor << std::endl;
  }
  if(!checkChanges())
    return false;
  update();
  return true;
}

int main(int argc, char **argv){
  const char* fb = argc >= 2 ? argv[1] : "/dev/vgfbmx";

  QApplication app( argc, argv );

  FBViewer viewer;
  if(!viewer.setFB(fb))
    return 1;
  viewer.show();

  return app.exec();
}
