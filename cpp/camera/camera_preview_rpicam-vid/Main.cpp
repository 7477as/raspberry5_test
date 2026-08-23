#include "CameraPreview.h"

int main() {
    CameraPreview preview;
    if (!preview.Init(1920, 1080, 30)) {
        return -1;
    }
    return preview.Run();
}
