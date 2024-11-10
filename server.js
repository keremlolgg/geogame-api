const express = require("express");
const axios = require("axios");
require("dotenv").config();

const app = express();
app.use(express.json());

const DISCORD_logWEBHOOK_URL = process.env.log;
const LEADERBOARD_API_URL = process.env.leadboard; // Leaderboard API URL'si .env dosyasından alınıyor


// Mesajları almak için kullanılacak fonksiyon
async function getMessages() {
  try {
    // API'ye istek gönderiyoruz
    const response = await axios.get(`${LEADERBOARD_API_URL}/messages/1304934491709771890`);
    console.log("API Yanıtı:", response.data); // Yanıtı kontrol et

    // response.data'dan sadece 'content' değerini döndürüyoruz
    if (response.data && response.data.content) {
      return response.data.content;  // 'content' anahtarının değerini döndürüyoruz
    } else {
      throw new Error('Content verisi bulunamadı.');
    }
    
  } catch (error) {
    console.error("Mesaj alınırken hata oluştu:", error.response ? error.response.data : error.message);
    throw error;
  }
}


app.post("/send_message", async (req, res) => {
  const { message } = req.body;

  // Mesajın olup olmadığını kontrol et
  if (!message) {
    return res.status(400).send("Mesaj boş olamaz.");
  }

  try {
    // Discord webhook'a mesajı gönder
    const response = await axios.post(DISCORD_logWEBHOOK_URL, {
      content: message, // Burada içerik gönderiliyor
    });

    // Başarılı yanıt
    res.status(200).send("Mesaj Discord'a gönderildi!");
  } catch (error) {
    // Hata mesajı ve daha fazla detay
    console.error("Mesaj gönderilemedi:", error.response ? error.response.data : error.message);
    res.status(500).send("Mesaj gönderme hatası.");
  }
});

// Mesajları getirmek için yeni endpoint
app.get("/get_messages", async (req, res) => {
  try {
    // Sabit bir mesajı alıyoruz
    const message = await getMessages();
    res.status(200).json({ message: message });
  } catch (error) {
    res.status(500).send("Mesaj alınırken hata oluştu.");
  }
});


// Sunucuyu başlat
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Sunucu çalışıyor`);
});
