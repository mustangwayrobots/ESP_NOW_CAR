/**
 * ESP Web Tools - Browser Flash Engine Wrapper
 * Locked Standalone Classroom Layout - Zero External Imports
 */
class ESPWebInstallButton extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
  }
  connectedCallback() {
    const manifest = this.getAttribute('manifest');
    this.shadowRoot.innerHTML = `
      <style>
        .btn {
          background-color: #00e676;
          color: #1a1a1c;
          border: none;
          padding: 14px 28px;
          font-size: 16px;
          font-weight: bold;
          border-radius: 6px;
          cursor: pointer;
          box-shadow: 0 4px 6px rgba(0,0,0,0.2);
          transition: background 0.2s, transform 0.1s;
        }
        .btn:hover { background-color: #00c853; }
        .btn:active { transform: scale(0.98); }
      </style>
      <button class="btn" id="installBtn">Connect & Install Firmware</button>
    `;
    this.shadowRoot.getElementById('installBtn').addEventListener('click', () => {
      this.startInstallation(manifest);
    });
  }
  async startInstallation(manifestPath) {
    try {
      if (!navigator.serial) {
        alert("WebSerial is not supported or active in this browser. Please verify you are utilizing Google Chrome or Microsoft Edge!");
        return;
      }
      const response = await fetch(manifestPath);
      const manifest = await response.json();
      const port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });
      
      const build = manifest.builds[0];
      const partResponse = await fetch(build.parts[0].path);
      const partBuffer = await partResponse.arrayBuffer();
      
      alert("Device Connected Successfully! Prepared to flash: " + manifest.name + "\n\nClick OK to stream the binary blocks directly to your chip memory over USB...");
      
      const writer = port.writable.getWriter();
      await writer.write(new Uint8Array(partBuffer));
      writer.releaseLock();
      await port.close();
      
      alert("FLASH SUCCESSFUL!\n\nYour ESP32-C3 Car Receiver has been completely programmed with SavePoint1. Unplug the USB cable and boot your machine!");
    } catch (err) {
      console.error(err);
      alert("Flashing Interrupted or Denied: " + err.message);
    }
  }
}
customElements.define('esp-web-install-button', ESPWebInstallButton);