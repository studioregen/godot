#ifndef FG_MODULE_H
#define FG_MODULE_H

#include "core/reference.h"

class FGCollisionModule : public Reference {
    GDCLASS(FGCollisionModule, Reference);
private:

protected:
    static void _bind_methods();

public:
    FGCollisionModule();
};

#endif
