# Video Konuşma Metni (Türkçe, sade, sohbet havasında)

**Proje:** BLM 4140 — Wi-Fi Başarım Analizi
**Hedef süre:** ~10 dakika
**Stil:** Kameraya doğrudan anlat. Kısa cümleler. Metni okuma, anlat.

`[1:00]` gibi zamanlar sadece kabaca yön gösterici.

---

## [0:00] Kendim

Selam! Ben Ozan Orhan. Öğrenci numaram 21011077.

Bu video, BLM 4140 — Kablosuz ve Mobil Ağlar dersi için yaptığım
dönem projesi. Hocam Dr. Öğretim Üyesi Mehmet Şükrü Kuran.

Bugün size ne yaptığımı kısaca anlatacağım.

## [0:45] Proje neyle ilgili?

Proje Wi-Fi'la ilgili. Tek bir soruya cevap arıyorum:

> Aynı Wi-Fi'a aynı anda birçok cihaz bağlanınca hıza ne oluyor?

Çok yıl önce, Bianchi adında bir araştırmacı şunu söylemiş:
*Aynı Wi-Fi'ı paylaşan cihaz sayısı arttıkça toplam hız düşer.*
Çünkü cihazlar birbirine çarpışıyor (collision) ve zaman boşa
gidiyor.

Ama Bianchi bunu 2000'de yazmış. Bugün Wi-Fi'da yeni özellikler
var — EDCA, A-MPDU, Block-ACK, TXOP. Ben de merak ettim: Bianchi
hâlâ haklı mı? Yoksa yeni özellikler sorunu çözmüş mü?

Aynı zamanda iki farklı paylaşma yöntemini karşılaştırıyorum:

* **EDCA** — normal yöntem.
* **EDCA + RTS/CTS** — aynısı, ama her cihaz önce "konuşabilir
  miyim?" diye soruyor ve "evet" cevabını bekliyor.

## [2:00] Ne yaptım — yöntem

**NS-3** adında bir program kullandım. Bu bir ağ simülatörü.
Küçük bir C++ dosyası yazıyorsunuz, program bilgisayarınızda
gerçek bir Wi-Fi ağını simüle ediyor.

Kurulum çok basit:

* Bir tane Wi-Fi router ("AP")
* Etrafında 4, 8, 12 veya 20 küçük cihaz ("STA")
* Tüm cihazlar AP'ye çok yakın (1 metre)
* Hepsi aynı anda veri yüklüyor (upload), **TCP** ile
* Üç farklı yük denedim: maksimum Wi-Fi hızının **%50, %80,
  %90**'ı

Yani 4 × 2 × 3 = **24 farklı test**.

Sonuçlar dalgalı çıkmasın diye her testi **5 kez** farklı
rastgele sayılarla koşturdum ve ortalamasını aldım. Toplamda
bilgisayar 120 kez simülasyon yaptı. Yaklaşık 20 dakika sürdü.

Sonra küçük bir Python betiğiyle grafikleri ve tabloları
ürettim.

## [3:00] Koda hızlıca bir bakalım

Şimdi C++ dosyasını açayım, içinde ne var göstereyim.
*(Editor'de `wifi-project.cc`'yi aç.)*

Tek bir dosya, yaklaşık 200 satır. Hepsini okumayacağım, sadece
önemli kısımları göstereceğim.

**En yukarıda** komut satırından üç şey alıyorum:

```cpp
uint32_t    numSTA           = 4;       // 4, 8, 12, 20
std::string macMechanism     = "EDCA";  // "EDCA" veya "RTSCTS"
double      totalLoadPercent = 50.0;    // 50, 80, 90
```

Yani 24 testin hepsi aynı dosya ile çalışıyor, ben sadece
çalıştırırken sayıları değiştiriyorum.

**RTS/CTS için** küçük bir numara kullandım. NS-3'te bir "eşik"
var — paket bu eşikten büyükse RTS/CTS kullanıyor. RTS/CTS
istediğimde eşiği 100 byte yapıyorum (her paket 100 byte'tan
büyük, yani her şeyde RTS/CTS açık). Saf EDCA istediğimde eşiği
65535 yapıyorum, hiçbir şey tetiklemiyor.

```cpp
Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                   UintegerValue(100));   // AÇIK
// veya
                   UintegerValue(65535)); // KAPALI
```

**Projenin istediği dört MAC özelliği** — EDCA, A-MPDU, Block-ACK
ve TXOP — her birini şu şekilde açıyorum:

* EDCA otomatik:
  `wifi.SetStandard(WIFI_STANDARD_80211n)` der demez EDCA açık.
* A-MPDU (frame aggregation) tek satır:
  `BE_MaxAmpduSize = 65535`. Bu, birçok küçük çerçeveyi büyük
  birinin içine paketliyor.
* Block-ACK A-MPDU ile **bedava** geliyor — standart zorunlu
  yapıyor, NS-3 kendi açıyor.
* TXOP'u ben elle ayarladım. Varsayılan 0, yani her sırada tek
  paket. Ben 3.2 ms yaptım, böylece cihaz bir sırada birden
  fazla birleştirilmiş paket gönderebiliyor:

  ```cpp
  Config::Set(".../BE_Txop/TxopLimits", StringValue("3200us"));
  ```

**Trafik için** her STA'da TCP ile bir `OnOffApplication`
çalışıyor. STA başına hız toplam yükün STA sayısına bölünmüş
hali; toplam talep aynı kalıyor:

```cpp
double perStaMbps = totalOfferedMbps / numSTA;
```

**En sonda** AP'nin kaç byte aldığını okuyup tek bir CSV satırı
basıyorum:

```cpp
std::cout << numSTA << "," << macMechanism << ","
          << totalLoadPercent << "," << thrMbps << std::endl;
```

Hepsi bu kadar. Küçük bir bash betiği (`run_all.sh`) 120
kombinasyonu sırayla koşturup her satırı `results_raw.csv`'ye
ekliyor. Sonra bir Python betiği grafikleri çiziyor.

## [5:00] Sonuçlara bakalım

Grafikleri tek tek inceleyelim.

### Şekil 1 — %50 yük (rahat yük)

Burada Wi-Fi pek yorgun değil, sadece %50 yükte.

İki şey görüyoruz:
- Mavi çizgi (EDCA) ve kırmızı çizgi (RTS/CTS) **çok yakın**.
  Siyah çubuklar (hata barı) büyük. Yani bu durumda hangisini
  seçtiğiniz çok önemli değil.
- 20 cihazda iki çizgi de aşağı düşüyor. Yani düşük yükte bile
  çok cihaz olmak sorun.

### Şekil 2 — %80 yük (yoğun)

Şimdi %80 yükteyiz. Wi-Fi yoğun.

Büyük sürpriz: kırmızı çizgi (RTS/CTS) mavi çizginin (EDCA)
**üstünde**, *her* cihaz sayısında. Hatta sadece 4 cihazda
bile!

Bu, ders kitabının söylediğinin tersi. Kitap der ki RTS/CTS
az cihazda zarar verir.

### Şekil 3 — %90 yük (çok yoğun)

Aynı manzara, ama daha güçlü. RTS/CTS çok daha iyi. 20 cihazda
yaklaşık 38 Mbit/s veriyor, ama EDCA sadece 28 veriyor. Büyük
fark.

### Şekil 4 — Hepsi bir arada

Bu son grafik altı çizgiyi tek resimde topluyor. Üç şey göze
çarpıyor:
1. **Her çizgi aşağı iniyor** cihaz sayısı arttıkça. Yani
   Bianchi haklıymış, modern Wi-Fi'da bile!
2. %80 ve %90 yükte **kırmızı çizgiler her zaman mavinin
   üstünde.** RTS/CTS kazanıyor.
3. %50 yükte iki çizgi neredeyse aynı.

## [7:30] Ne bekledim, ne gördüm?

Simülasyondan önce şunu bekliyordum:

* Az cihazda → RTS/CTS biraz daha kötü (ek mesajlar, kazanç yok).
* Çok cihazda → RTS/CTS daha iyi (çarpışmaları kısaltıyor).

Aslında gördüğüm:

* Az cihaz + düşük yük → iki yöntem birbirine çok yakın. ✓
* Çok cihaz + yüksek yük → RTS/CTS daha iyi. ✓
* **Ama yüksek yükte sadece 4 cihazla bile RTS/CTS daha iyi!**
  Bunu beklemiyordum.

Neden böyle? İki sebep:

1. TCP ile her cihaz **uzun paketler** gönderiyor. İki uzun
   paket çarpışınca çok zaman kaybediyoruz. RTS/CTS
   çarpışmaları **kısa** yapıyor.
2. AP'nin de TCP "teşekkür" mesajları (ACK) yollaması lazım.
   Çok cihaz upload yaparken AP kanalı zor kapıyor. RTS/CTS
   AP'ye "hey, sıra bende!" demesine yardım ediyor.

Yani 4 cihazda bile, kısa çarpışmaların kazancı RTS/CTS'in ek
mesaj maliyetinden büyük çıkıyor.

## [9:00] Sonuç

Peki ne öğrendim?

* **Bianchi 25 yıl sonra hâlâ haklı.** Cihaz sayısı arttıkça
  toplam hız düşüyor. Modern Wi-Fi özellikleri biraz yardım
  ediyor ama sorunu tamamen çözmüyor.
* **Düşük yükte EDCA mı RTS/CTS mi pek farketmiyor.**
* **Yüksek yükte RTS/CTS her yerde kazanıyor.** Az cihazda
  bile. Ders kitabı için sürpriz ama TCP upload için mantıklı.
* **Pratik tavsiye:** çok kişi aynı anda upload yapıyorsa —
  örneğin 20 öğrencinin hep birlikte aynı şeyi yüklediği bir
  sınıf — RTS/CTS'i açın. Gerçekten yardımcı oluyor.

## [9:45] Teşekkür

Benden bu kadar. İzlediğiniz için teşekkür ederim.

Tüm dosyalar — kaynak kod, sonuçlar, grafikler, İngilizce ve
Türkçe rapor — proje ZIP'inde var.

Sorularınız olursa lütfen e-posta atın. Teşekkürler!
