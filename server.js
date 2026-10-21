const express = require("express");
const path = require("path");
const fs = require("fs");

const app = express();
const PORT = process.env.PORT || 3000;
const API_KEY = process.env.API_KEY || "cambiame";
const MAX_HISTORY = 3000;

let device = null;   // última vez que el ESP32 se comunicó
let pos = null;      // última posición con fix
let history = [];    // recorrido

app.use(express.json({ limit: "10kb" }));

// Página: funciona con index.html junto a server.js o dentro de /public
const candidatos = [path.join(__dirname, "public", "index.html"), path.join(__dirname, "index.html")];
const paginaPath = candidatos.find(p => fs.existsSync(p));
app.get("/", (req, res) => {
  if (!paginaPath) return res.status(500).send("Falta index.html junto a server.js");
  res.sendFile(paginaPath);
});

// El ESP32 manda acá (con o sin fix)
app.post("/api/update", (req, res) => {
  if (req.get("x-api-key") !== API_KEY) return res.status(401).json({ error: "clave incorrecta" });
  const b = req.body || {};
  const now = Date.now();
  device = { receivedAt: now, sats: Number(b.sats) || 0 };

  if (b.fix) {
    if (typeof b.lat !== "number" || typeof b.lng !== "number") {
      return res.status(400).json({ error: "lat/lng invalidos" });
    }
    pos = { lat: b.lat, lng: b.lng, speed: Number(b.speed) || 0, alt: Number(b.alt) || 0,
            sats: Number(b.sats) || 0, hdop: Number(b.hdop) || 0, receivedAt: now };
    history.push([b.lat, b.lng]);
    if (history.length > MAX_HISTORY) history.shift();
  }
  res.json({ ok: true });
});

// La página consulta acá
app.get("/api/location", (req, res) => {
  res.set("Cache-Control", "no-store");
  const now = Date.now();
  res.json({
    deviceAgeMs: device ? now - device.receivedAt : null,
    deviceSats: device ? device.sats : 0,
    fix: !!pos,
    pos: pos ? { ...pos, ageMs: now - pos.receivedAt } : null
  });
});

app.get("/api/history", (req, res) => {
  res.set("Cache-Control", "no-store");
  res.json(history);
});

app.post("/api/clear", (req, res) => {
  if (req.get("x-api-key") !== API_KEY) return res.status(401).json({ error: "clave incorrecta" });
  history = [];
  res.json({ ok: true });
});

app.listen(PORT, () => console.log("Servidor GPS en puerto " + PORT));
