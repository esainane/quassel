#ifndef BRINEADAPTER_H
#define BRINEADAPTER_H

#include <QThread>
class BrineAdapter : public QThread
{
public:
    BrineAdapter();

    void run();
};

extern BrineAdapter *brineAdapter;

#endif // BRINEADAPTER_H
