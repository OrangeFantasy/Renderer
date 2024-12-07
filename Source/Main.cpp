 #include "Render/Renderer.h"
//
// #include <memory>

#include "Core/Core.h"

struct TA
{
    TA() { P = new int(1); }
    ~TA()
    {
        delete P;
        P = nullptr;
    }

    int* P = nullptr;
};

int main(int argc, char** argv)
{
    auto Ta = new TA;
    std::cout << Ta->P << std::endl;

    {
        TRefCountPtr<TA> Ref1(Ta);
        std::cout << Ta->P << "  " << Ref1.GetRefCount() << std::endl;

        TRefCountPtr<TA> Ref2;
        Ref2.operator=(std::move(Ref1));
        std::cout << Ta->P << "  " << Ref2.GetRefCount() << std::endl;
    }
    std::cout << Ta->P << std::endl;

     ARenderer Renderer = ARenderer();
     Renderer.MainTick();

    return 0;
}