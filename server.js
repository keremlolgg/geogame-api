const express = require("express");
const axios = require("axios");
const fetch = require("node-fetch");
const fs = require("fs");
const path = require('path');
const https = require("https");
const FormData = require('form-data');
require("dotenv").config();
const app = express();
app.use(express.json());

const readFileAsync = async (filePath) => {
  try {
    const data = await fs.promises.readFile(filePath, 'utf8');
    return JSON.parse(data);
  } catch (error) {
    if (error.code === 'ENOENT') {
      console.warn(`${filePath} dosyası bulunamadı, yeni dosya oluşturulacak.`);
      return { users: [] }; // Varsayılan boş kullanıcı listesi döndür
    }
    throw error; // Diğer hataları yükselt
  }
};

const updateDatabase = async (userData) => {
  try {
    const db = await readFileAsync('database.json');

    const userIndex = db.users.findIndex((user) => user.name === userData.name);

    if (userIndex !== -1) {
      const existingPuan = parseInt(db.users[userIndex].puan);
      const newPuan = parseInt(userData.toplampuan);

      if (existingPuan < newPuan) {
        // Kullanıcıyı güncelle
        db.users[userIndex] = {
          name: userData.name,
          puan: userData.toplampuan,
          mesafedogru: userData.mesafedogru,
          mesafeyanlis: userData.mesafeyanlis,
          bayrakdogru: userData.bayrakdogru,
          bayrakyanlis: userData.bayrakyanlis,
          baskentdogru: userData.baskentdogru,
          baskentyanlis: userData.baskentyanlis,
          mesafepuan: userData.mesafepuan,
          bayrakpuan: userData.bayrakpuan,
          baskentpuan: userData.baskentpuan,
        };
        await fs.promises.writeFile('database.json', JSON.stringify(db, null, 2), 'utf8');
        console.log(`Puan güncellendi: ${userData.name} - ${userData.toplampuan}`);
      } else {
        console.log(`Puan güncellenmedi, yeni veriler daha düşük: ${userData.name} - ${userData.toplampuan}`);
      }
    } else {
      // Yeni kullanıcıyı ekle
      db.users.push({
        name: userData.name,
        puan: userData.toplampuan,
        mesafedogru: userData.mesafedogru,
        mesafeyanlis: userData.mesafeyanlis,
        bayrakdogru: userData.bayrakdogru,
        bayrakyanlis: userData.bayrakyanlis,
        baskentdogru: userData.baskentdogru,
        baskentyanlis: userData.baskentyanlis,
        mesafepuan: userData.mesafepuan,
        bayrakpuan: userData.bayrakpuan,
        baskentpuan: userData.baskentpuan,
      });
      await fs.promises.writeFile('database.json', JSON.stringify(db, null, 2), 'utf8');
      console.log(`Yeni kullanıcı eklendi: ${userData.name} - ${userData.toplampuan}`);
    }

    console.log("Veritabanı başarıyla güncellendi.");
  } catch (err) {
    console.error('Veritabanı dosyası yazma hatası:', err);
  }
};

const sendFileToDiscord = async (filePath, channelId) => {
  const file = fs.createReadStream(filePath);
  const formData = new FormData();
  formData.append('file', file);

  const response = await axios.post(
    `https://discord.com/api/v10/channels/${channelId}/messages`,
    formData,
    {
      headers: {
        'Authorization': `Bot ${process.env.bot_token}`,
        'Content-Type': 'multipart/form-data',
        ...formData.getHeaders(),
      },
    }
  );
  console.log("Dosya başarıyla gönderildi!");
};

const sendMessageToDiscord = async (message, channelId) => {
  if (!channelId || typeof channelId !== 'string' || channelId.trim() === '') {
    console.error("Geçersiz kanal kimliği.");
    sendErrorToChannel("Geçersiz kanal kimliği.");
    throw new Error("Geçersiz kanal kimliği.");
  }

  try {
    await axios.post(
      `https://discord.com/api/v10/channels/${channelId}/messages`,
      { content: message },
      {
        headers: {
          'Authorization': `Bot ${process.env.bot_token}`,
          'Content-Type': 'application/json',
        },
      }
    );
    console.log("Mesaj başarıyla gönderildi!");
  } catch (error) {
    console.error("Mesaj gönderilemedi: ", error.response ? error.response.data : error.message);
    sendErrorToChannel("Mesaj gönderilemedi: ", error.response ? error.response.data : error.message);
    throw new Error("Mesaj gönderilemedi. Hata: " + (error.response ? error.response.data : error.message));
  }
};

async function sendErrorToChannel(errorMessage) {
  try {
    // Discord API'ye POST isteği gönderiyoruz
    const response = await axios.post(
      `https://discord.com/api/v10/channels/${process.env.server_api_error_log}/messages`,
      {
        content: `Hata oluştu: ${errorMessage}`, // Göndermek istediğiniz hata mesajı
      },
      {
        headers: {
          Authorization: `Bot ${process.env.bot_token}`, // Bot tokenini header'a ekliyoruz
        },
      }
    );

    console.log("Hata bilgisi kanala gönderildi.");
  } catch (error) {
    console.error("Hata mesajı gönderilirken bir sorun oluştu:", error.message);
  }
}

async function getMessages() {
  try {
    const response = await axios.get(
      `https://discord.com/api/v10/channels/${process.env.kullanici_puanlari}/messages`,
      {
        headers: {
          Authorization: `Bot ${process.env.bot_token}`,
        },
        params: {
          limit: 1,
        },
      }
    );

    if (response.data && response.data.length > 0) {
      const lastMessage = response.data[0]; // İlk mesaj
      if (lastMessage.attachments && lastMessage.attachments.length > 0) {
        const fileUrl = lastMessage.attachments[0].url;
        console.log("Dosya URL'si:", fileUrl);
        return fileUrl;  // Dosya URL'sini döndür
      } else {
        throw new Error("Mesajda dosya yok.");
      }
    } else {
      throw new Error("Mesaj bulunamadı.");
    }
  } catch (error) {
    console.error("Mesaj alınırken hata oluştu:", error.message);
    sendErrorToChannel("Mesaj alınırken hata oluştu: " + error.message);
    throw error; // Hata durumunda tekrar fırlat
  }
}

async function downloadFile(fileUrl, filePath) {
  return new Promise((resolve, reject) => {
    https
      .get(fileUrl, (res) => {
        if (res.statusCode !== 200) {
          // Eğer HTTP yanıtı 200 değilse, hata olarak değerlendir.
          return reject(new Error(`Dosya indirilemedi. HTTP Durumu: ${res.statusCode}`));
        }

        const fileStream = fs.createWriteStream(filePath);
        res.pipe(fileStream);

        fileStream.on("finish", () => {
          resolve(filePath); // Dosya başarıyla indirildi.
        });

        fileStream.on("error", (error) => {
          reject(error); // Yazma sırasında hata oluşursa
        });
      })
      .on("error", (error) => {
        reject(error); // HTTPS isteğinde hata oluşursa
      });
  }).catch(async (error) => {
    console.error("Dosya indirilemedi, sabit JSON dosyası yazılıyor:", error.message);
    sendErrorToChannel("Dosya indirilemedi, sabit JSON dosyası yazılıyor:", error.message);

    // Sabit JSON verisi
    const fallbackData = {
      mesaj: "Dosya indirme hatası dolayı sabit mesaj",
      name: "Test Device",
      dil: "Türkçe",
      surum: "1.2.1",
      ulke: "Türkiye",
      toplampuan: "0",
      mesafedogru: "0",
      mesafeyanlis: "0",
      bayrakdogru: "0",
      bayrakyanlis: "0",
      baskentdogru: "0",
      baskentyanlis: "0",
      mesafepuan: "0",
      bayrakpuan: "0",
      baskentpuan: "0"
    };

    // Sabit JSON'u dosyaya yaz
    try {
      await fs.promises.writeFile(filePath, JSON.stringify(fallbackData, null, 2), 'utf8');
      console.log("Sabit JSON dosyaya yazıldı:", filePath);
      sendErrorToChannel("Sabit JSON dosyaya yazıldı:", filePath);
      return filePath; // Sabit dosyanın yolunu döndür
    } catch (writeError) {
      console.error("Sabit JSON dosyası yazılamadı:", writeError.message);
      sendErrorToChannel("Sabit JSON dosyası yazılamadı:", writeError.message);
      throw new Error("Dosya indirilemedi ve sabit JSON dosyası yazılamadı.");
    }
  });
}

app.post("/newuser", async (req, res) => {
  const { message } = req.body;

  // Mesajın olup olmadığını kontrol et
  if (!message) {
    return res.status(400).send("Mesaj boş olamaz.");
    sendErrorToChannel("Mesaj boş olamaz.");
  }
  try {
    sendMessageToDiscord(message,process.env.new_user_log)
  } catch (error) {
    // Eğer bot tokeni ile mesaj gönderilemezse, webhook ile mesaj gönder
    console.error(
      "Mesaj gönderilemedi, ",
      error.response ? error.response.data : error.message
    );
    sendErrorToChannel(
      "Mesaj gönderilemedi, ",
      error.response ? error.response.data : error.message
    );
  }
});

app.post("/post_leadboard", async (req, res) => {
  const { message } = req.body;

  // Gelen mesajı kontrol et
  if (!message) {
    sendErrorToChannel("Mesaj boş olamaz.");
    return res.status(400).send("Mesaj boş olamaz.");
  }

   let obj;
  try {
    obj = JSON.parse(message);
  } catch (error) {
    console.error("Gelen mesaj JSON formatında değil:", error.message);
    sendErrorToChannel("Geçersiz JSON formatı.");
    return res.status(400).send("Geçersiz JSON formatı.");
  }
  // Gelen JSON'dan dosyayı indir
  const fileUrl = await getMessages(); // İndirilecek dosyanın URL'sini alın
  const filePath = "database.json"; // Dosyanın kaydedileceği yerel yol
  try {
    await downloadFile(fileUrl, filePath);
    console.log("Dosya başarıyla indirildi: ", filePath);
  } catch (error) {
    sendErrorToChannel("Dosya indirilemedi.");
    return res.status(500).send("Dosya indirilemedi.");
  }
    try {
    await updateDatabase(obj);
  } catch (error) {
    console.error("Veritabanı güncellenemedi:", error.message);
    sendErrorToChannel("Veritabanı güncellenemedi.");
    return res.status(500).send("Veritabanı güncellenemedi.");
  }
  try {
    // Discord'a mesaj gönderme
    if (process.env.bot_token) {
      // Veritabanı dosyasını yeniden oku
      const updatedDb = await readFileAsync('database.json');

      if (process.env.kullanici_puanlari) {
        sendFileToDiscord(filePath, process.env.kullanici_puanlari)
      } else {
        throw new Error("Kullanıcı puanları kanal ID'si eksik.");
      }

      // İkinci kanal (puan log kanalı)
      if (process.env.puan_log) {
        sendMessageToDiscord("Tablo Güncellendi Veri: " + "```json\n"+message+"```", process.env.puan_log)
      } else {
        throw new Error("Puan log kanal ID'si eksik.");
      }

      return res.status(200).send("Mesajlar ve dosya Discord'a başarıyla gönderildi!");
    } else {
      throw new Error("Bot tokeni eksik.");
    }
  } catch (error) {
    console.error("Veritabanı güncellenemedi: ", error.message);
    sendErrorToChannel("Veritabanı güncellenemedi.", error.message);
    return res.status(500).send("Veritabanı güncellenemedi.");
  }
});

app.get("/get_leadboard", async (req, res) => {
  try {
    // getMessages() fonksiyonuyla dönen URL'yi alıyoruz
    const fileUrl = await getMessages();
    const filePath = "./user.json"; // İndirilecek dosyanın yerel yolu

    // Dosyayı indir
    await downloadFile(fileUrl, filePath);

    // Dosya indirildikten sonra, kullanıcıya dosyayı gönder
    res.download(filePath, "downloaded_file.json", (err) => {
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
    sendErrorToChannel("Dosya indirilirken hata oluştu:", error);
    res.status(500).send("Dosya indirilirken hata oluştu.");
  }
});

// Sunucuyu başlat
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Api çalışıyor`);
});
