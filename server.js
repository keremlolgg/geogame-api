const express = require("express");
const axios = require("axios");
const fetch = require('node-fetch'); 
const fs = require('fs');
const https = require('https');
require("dotenv").config();

const app = express();
app.use(express.json());


// Mesajları almak için kullanılacak fonksiyon
async function getMessages() {
  try {
    // API'ye istek gönderiyoruz
    const response = await axios.get(`${process.env.leadboard}/messages/1304934491709771890`);
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

async function downloadFile(fileUrl, filePath) {
  return new Promise((resolve, reject) => {
    https.get(fileUrl, (res) => {
      const fileStream = fs.createWriteStream(filePath);
      res.pipe(fileStream);

      fileStream.on('finish', () => {
        resolve(filePath);
      });

      fileStream.on('error', (error) => {
        reject(error);
      });
    }).on('error', (error) => {
      reject(error);
    });
  });
}

app.post("/send_message", async (req, res) => {
  const { message } = req.body;

  // Mesajın olup olmadığını kontrol et
  if (!message) {
    return res.status(400).send("Mesaj boş olamaz.");
  }

  try {
    // Discord webhook'a mesajı gönder
    const response = await axios.post(process.env.log, {
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

app.post("/newuser", async (req, res) => {
  const { message } = req.body;

  // Mesajın olup olmadığını kontrol et
  if (!message) {
    return res.status(400).send("Mesaj boş olamaz.");
  }

  try {
    // Discord webhook'a mesajı gönder
    const response = await axios.post(process.env.newuser, {
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

// Dosya indirme için yeni endpoint
app.get("/download_file", async (req, res) => {
  try {
    // getMessages() fonksiyonuyla dönen URL'yi alıyoruz
    const fileUrl = await getMessages();
    const filePath = './users.json';  // İndirilecek dosyanın yerel yolu

    // Dosyayı indir
    await downloadFile(fileUrl, filePath);

    // Dosya indirildikten sonra, kullanıcıya dosyayı gönder
    res.download(filePath, 'downloaded_file.json', (err) => {
      if (err) {
        console.error("Dosya gönderilirken hata oluştu:", err);
        res.status(500).send("Dosya gönderilemedi.");
      }
      // İndirilen dosyayı sil
      fs.unlink(filePath, (unlinkErr) => {
        if (unlinkErr) {
          console.error("Dosya silinirken hata oluştu:", unlinkErr);
        }
      });
    });
  } catch (error) {
    console.error("Dosya indirilirken hata oluştu:", error);
    res.status(500).send("Dosya indirilirken hata oluştu.");
  }
});

// Sunucuyu başlat
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Api çalışıyor`);
});
