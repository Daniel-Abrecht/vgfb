/*
 * Copyright (C) 2017 Daniel Patrick Abrecht
 * 
 * This program is dual licensed under the MIT License and
 * the GNU General Public License v2.0
 */

#include <linux/fb.h>
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
  update();
}

void FBViewer::update(){
  if(!memory_size)
    return;
  label->setPixmap(QPixmap::fromImage(QImage(memory,var->xres,var->yres,(QImage::Format)format)));
}

bool FBViewer::setFB(const char* path){
  int fb = ::open(path,O_RDWR);
  if( fb == -1 ){
    std::cerr << "open failed: " << strerror(errno) << std::endl;
    return false;
  }
  struct fb_var_screeninfo var;
  if( ioctl(fb, FBIOGET_VSCREENINFO, &var) == -1 ){
    std::cerr << "FBIOGET_VSCREENINFO failed: " << strerror(errno) << std::endl;
    ::close(fb);
    return false;
  }
  format = QImage::Format::Format_RGBX8888; // TODO
  std::size_t size = var.xres_virtual * var.yres_virtual * 4;
  unsigned char* mem = (unsigned char*)mmap(0, size, PROT_READ, MAP_SHARED, fb, 0);
  if( !mem || mem==MAP_FAILED ){
    std::cerr << "mmap failed: " << strerror(errno) << std::endl;
    return false;
  }
  if( memory && memory != MAP_FAILED )
    munmap(memory,memory_size);
  if( this->fb != -1 )
    ::close(this->fb);
  this->memory = mem;
  this->memory_size = size;
  *this->var = var;
  this->fb = fb;
  resize(var.xres,var.yres);
  label->resize(var.xres,var.yres);
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
