![ColorFromPoint 실행 화면](./Images/ColorFromPoint-demo.gif)
# 📌 ColorFromPoint<br>
Windows Legacy API와 후킹을 이용한 컬러 픽커입니다.<br>
이 프로젝트는 MIT 라이선스 하에 제공되어 누구나 자유롭게 사용, 수정, 배포할 수 있습니다. 개인 또는 상업적 용도로도 활용이 가능합니다.<br>
<br><br>
## 📝 Description<br>
"ColorFromPoint"는 이름 그대로 마우스 커서가 위치한 지점의 색상을 조사하는 프로그램입니다.<br>
마우스를 기준으로 일정 범위를 캡처한 후 픽셀 단위로 값을 추출할 수 있습니다.<br>
작업 영역 내에 에디트 컨트롤을 배치하여 RGB, HSV, HEX 값을 표시합니다.<br>
알파 채널은 지원하지 않습니다.<br>
<br>
다음과 같은 단축키를 지원합니다.<br>
- Ctrl + Alt + 3 : 마우스 주변 영역을 캡처합니다.
- Ctrl + Alt + 4 : 마우스 커서가 위치한 지점의 색상값을 추출합니다
- Alt + Wheel Up(Down) : 이미지를 확대하거나 축소할 수 있습니다.
<br><br>
## 🚀 Getting Started<br>
### 🔧 Dependencies<br>
Windows 10 이상<br>
MyApiDll.dll<br>
<br>
### 📥 Installing<br>
[Releases](https://github.com/stdsic/ColorFromPoint/releases) 페이지에서 최신 버전을 확인하실 수 있습니다.<br>
압축 해제 후 `ColorFromPoint.exe` 또는 `ColorFromPointInstaller.exe`를 실행합니다.<br>
설치 파일(ColorFromPointInstaller.exe)을 다운로드한 경우 안내에 따라 안전하게 실행 파일(ColorFromPoint.exe)을 설치하시면 됩니다.<br>
<br>
### ▶️ Executing program<br>
- ColorFromPoint.exe<br>
<br><br>
## ❓ Help<br>
- 기본적으로 알파 채널은 포함되지 않으며 투명도 정보는 버려집니다.
- 알파 채널이 RGB 값과 곱해지는 프리멀티플 알파(Premultiplied alpha)가 적용된 경우 값이 제대로 조사되지 않습니다.
  - Direct 2D, GDI+ 등이 이에 해당됩니다.
- 웹의 RGBA는 대부분 비프리멀티플(non-Premultiplied alpha) 방식입니다.
- 현대의 모니터는 대체로 감마가 적용된 sRGB 색 공간을 표준으로 하며 위 프로그램에서 조사하는 색상 값도 이 표준을 따릅니다.
- 조작 방법 및 더 자세한 정보는 프로그램 실행 후 [메뉴] - [프로그램 소개] 항목에서 확인하실 수 있습니다.
- CMYK 색 공간 변환식은 장치 의존적이므로 명확한 한계가 존재합니다. v1.1 이후 CMYK 변환식이 제거되었습니다.
<br><br>
## 👤 Authors<br>
- stdsic — @https://github.com/stdsic/ColorFromPoint<br>
<br><br>
## 📚 Version History<br>
- 1.1.1<br>
  - HSV 색 공간 추가
  - 프로그램 소개글 수정
  - 일부 메세지 로직 수정
- 1.1.0<br>
  - CMYK 색 공간 및 변환식 제거
- 1.0.2<br>
  - sRGB to CMYK 변환 함수 수정
  - 프로그램 메타 데이터 수정
- 1.0.1<br>
  - 메타 데이터 수정<br>
- 1.0.0<br>
  - 최초 릴리스<br>
<br><br>
## 🧾 License<br>
이 프로젝트는 [MIT License](LICENSE)로 라이선스되어 있습니다.<br>
자세한 정보는 LICENSE 파일을 참고하시기 바랍니다.<br>
<br>
