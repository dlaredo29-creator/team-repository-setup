# Hand Proximity Demo

From the repository folder, start a local server:

```sh
python3 -m http.server 8000
```

Open `http://localhost:8000/demo/`, choose **Start hand demo**, and allow camera access. Hold an open palm toward the camera and move it closer or farther. The palm-width signal is relative image size, not centimeters: below `0.12` shows NOBODY, `0.12` to `0.35` shows DETECTED, and `0.35` or above shows CLOSE.

Press **D** for the current state and signal values, **B** for hand landmarks, or **V** to toggle the camera preview. The webcam and hand model run in the browser; frames are not saved.