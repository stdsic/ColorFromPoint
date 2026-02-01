#include "Color.h"
#include <math.h>
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define CLAMP(Min, Max, N) (((N) < (Min)) ? (Min) : ((N) < (Max)) ? (N) : (Max));

const float Color::Ratio = 1.f / 255.f;
const Color Color::White(1.f, 1.f, 1.f);
const Color Color::Black(0.f, 0.f, 0.f);
const Color Color::WhiteGray(0.9f, 0.9f, 0.9f);
const Color Color::LightGray(0.75f, 0.75f, 0.75f);
const Color Color::Gray(0.5f, 0.5f, 0.5f);
const Color Color::Red(1.f, 0.f, 0.f);
const Color Color::Green(0.f, 1.f, 0.f);
const Color Color::Blue(0.f, 0.f, 1.f);
const Color Color::Yellow(1.f, 1.f, 0.f);
const Color Color::Cyan(0.f, 1.f, 1.f);
const Color Color::Magenta(1.f, 0.f, 1.f);


Color::Color(COLORREF ColorRef) : _bHSV(FALSE) {
    _R = ((BYTE)(ColorRef)) * Ratio;
    _G = ((BYTE)(((WORD)(ColorRef)) >> 8)) * Ratio;
    _B = ((BYTE)((ColorRef) >> 16)) * Ratio;
}

Color::Color(float R, float G, float B, BOOL bHSV) 
    : _bHSV(bHSV), _H(0.f), _S(1.f), _V(1.f)
{
    if(bHSV == FALSE){
        _R = CLAMP(0.f, 1.f, R);
        _G = CLAMP(0.f, 1.f, G);
        _B = CLAMP(0.f, 1.f, B);
    }else{
        _H = R;
        _S = G;
        _V = B;
    }
}

Color::~Color(){

}

float Color::MaxColor() const { return max(_R, max(_G, _B)); }

// Norm RGB -> RGB
COLORREF Color::ToColorRef(){
    float R = CLAMP(0.f, 1.f, _R);
    float G = CLAMP(0.f, 1.f, _G);
    float B = CLAMP(0.f, 1.f, _B);

    UINT RV = (UINT)(R * 255.999f);
    UINT GV = (UINT)(G * 255.999f);
    UINT BV = (UINT)(B * 255.999f);

    COLORREF ret = RGB(RV, GV, BV);

    return ret;
}

// HSV -> RGB
Color Color::ToColor(){
    float R = 0.f, G = 0.f, B = 0.f;

    int i = (int)(_H * 6.f);
    float f = _H * 6.f - i;
    float p = _V * (1.f - _S);
    float q = _V * (1 - f * _S);
    float t = _V * (1 - (1 - f) * _S);

    if(i>=6){ i = 0; }
    if(i==0){ R = _V, G = t, B = p; }
    if(i==1){ R = q, G = _V, B = p; }
    if(i==2){ R = p, G = _V, B = t; }
    if(i==3){ R = p, G = q, B = _V; }
    if(i==4){ R = t, G = p, B = _V; }
    if(i==5){ R = _V, G = p, B = q; }

    return Color(R,G,B);
}

void Color::ToHSV(){
    float BrightestValue = max(_R, max(_G, _B));
    float DarkestValue = min(_R, min(_G, _B));
    float Delta = BrightestValue - DarkestValue;

    _V = BrightestValue;
    _S = (BrightestValue == 0.f) ? 0.f : Delta / BrightestValue;
    if(Delta == 0.f){ _H = 0.f; }
    else{
        if(BrightestValue == _R){
            _H = fmodf((_G - _B) / Delta, 6.f);
        }else if(BrightestValue == _G){
            _H = ((_B - _R) / Delta) + 2.f;
        }else{
            _H = ((_R - _G) / Delta) + 4.f;
        }

        // 정규화
        _H /= 6.f;

        if(_H < 0.f){
            _H += 1.f;
        }
    }
}

const Color operator +(const float& Value, const Color& C) {
    return C + Value;
}

const Color operator -(const float& Value, const Color& C) {
    return C - Value;
}

const Color operator *(const float& Value, const Color& C) {
    return C * Value;
}

const Color operator /(const float& Value, const Color& C) {
    return C / Value;
}



