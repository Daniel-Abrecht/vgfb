#ifndef VIEWER_H
#define VIEWER_H

#include <QMainWindow>

class QLabel;
class QPixmap;
class QTimer;
class QScrollArea;

class FBViewer : public QMainWindow {
  Q_OBJECT
private:
  int fb;
  QLabel* label;
  QTimer* timer;
  struct fb_var_screeninfo* var;
  unsigned char* memory;
  long format;
  std::size_t memory_size;
  QScrollArea* scroll_area;
public:
  explicit FBViewer(QWidget *parent = 0);
  void resizeEvent(QResizeEvent* event);
  bool setFB(const char* path);
  bool checkChanges();
public slots:
  void update();
};

#endif
