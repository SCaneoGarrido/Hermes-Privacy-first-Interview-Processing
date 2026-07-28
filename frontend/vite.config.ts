import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// El backend de Crow no envia headers CORS. En vez de pedirle ese cambio,
// el frontend siempre llama a rutas bajo "/api/*" y Vite las reenvia al
// backend real sin reescribir el path -- el backend expone sus rutas bajo
// ese mismo prefijo "/api/v1/..." (ver backend/main.cpp), asi que llega tal
// cual. El navegador ve todo como el mismo origen (localhost:5173), asi que
// no hay preflight ni bloqueo de CORS. Ver docs/API_REQUIREMENTS.md seccion 4.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      "/api": {
        target: "http://127.0.0.1:18080",
        changeOrigin: true,
      },
    },
  },
});
