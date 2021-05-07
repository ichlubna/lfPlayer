#ifndef INPUTS_HDR
#define INPUTS_HDR
#include <map>
class Inputs
{
    public:
        enum Key : long
        {	ESC = 1,
            LMB = 2, RMB = 4, MMB = 8,
            W = 16, A = 32, S = 64, D = 128,
            ALT = 256, ENTER = 512, Z = 1024
        };
        std::map<long, bool> released;

        struct mousePosition
        {
            double x, y; 
        };

        //TODO double is probably useless overkill
 
        //signal to close app eg. clicking the cross icon
        bool close {false};

        mousePosition getMousePosition() const;
        double getMouseScroll() const;

        void setMousePosition(double x, double y);
        void setMouseScroll(double s);
        void press(Key key);
        void release(Key key);

        bool pressedAny() const {return keys!=0;}
        
        template <typename... Args>
        bool pressed(Args... args) const
        {
            long mask = composeKeys(args...);
            return (keys & mask) == mask;
        }
        template <typename T, typename... Args>
        long composeKeys(T keyFlag, Args... args) const
        {
            return keyFlag | composeKeys(args...);
        }
        template <typename T>
        long composeKeys(T t) const
        {
            return t;
        }

        template <typename... Args>
        bool pressedAfterRelease(Args... args)
        {
            bool isPressed = pressed(args...);
            long mask = composeKeys(args...);
            bool wasReleased = released[mask];
            if(isPressed)
            {
                released[mask] = false;
                if(wasReleased)
                    return true;
            }
            else
                released[mask] = true;
            return false;
        }
    private:
        //expecting max 32 keys, plus possible modifiers
        long keys {0};
        double mouseX {0}, mouseY {0}, mouseScroll {0};
};

#endif
