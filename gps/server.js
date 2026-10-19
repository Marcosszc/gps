const express = require("express");
const path = require("path");

const app = express();
const PORT = process.env.PORT || 3000;
const API_KEY = process.env.API_KEY || "cambiame";
const MAX_HISTORY = 3000;

let last = null;     // última posición
let history = [];    // recorrido

app.use(express.json({ limit: "10kb" }));
app.use(express.static(path.join(__dirname, "public")));

// El ESP32 manda la posición acá
app.post("/api/update", (req, res) => {
  if (req.get("x-api-key") !== API_KEY) return res.status(401).json({ error: "clave incorrecta" });
  const { lat, lng, speed = 0, alt = 0, sats = 0, hdop = 0 } = req.body || {};
  if (typeof lat !== "number" || typeof lng !== "number") {
    return res.status(400).json({ error: "lat/lng inválidos" });
  }
  last = { lat, lng, speed, alt, sats, hdop, receivedAt: Date.now() };
  history.push([lat, lng]);
  if (history.length > MAX_HISTORY) history.shift();
  res.json({ ok: true });
});

// La página pide la última posición
app.get("/api/location", (req, res) => {
  res.set("Cache-Control", "no-store");
  if (!last) return res.json({ valid: false });
  res.json({ valid: true, ...last, ageMs: Date.now() - last.receivedAt });
});

// Recorrido guardado
app.get("/api/history", (req, res) => {
  res.set("Cache-Control", "no-store");
  res.json(history);
});

// Borrar recorrido (requiere clave)
app.post("/api/clear", (req, res) => {
  if (req.get("x-api-key") !== API_KEY) return res.status(401).json({ error: "clave incorrecta" });
  history = [];
  res.json({ ok: true });
});

app.listen(PORT, () => console.log("Servidor GPS en puerto " + PORT));
