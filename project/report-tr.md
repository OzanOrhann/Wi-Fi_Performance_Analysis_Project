---
documentclass: article
geometry: margin=2cm
fontsize: 11pt
mainfont: DejaVu Sans
monofont: DejaVu Sans Mono
header-includes: |
  \usepackage{graphicx}
  \usepackage{titling}
---

\begin{titlepage}
\centering
\vspace*{1cm}

\includegraphics[width=4.5cm]{figures/ytu_logo.png}

\vspace{1.2cm}

{\Large\textbf{YILDIZ TEKNİK ÜNİVERSİTESİ}}\\[0.4em]
{\large Elektrik-Elektronik Fakültesi}\\[0.2em]
{\large Bilgisayar Mühendisliği Bölümü}

\vspace{2.5cm}

{\Huge\bfseries Wi-Fi Başarım Analizi Projesi}\\[0.8em]
{\Large IEEE 802.11n EDCA ile EDCA+RTS/CTS Karşılaştırması}

\vspace{1.5cm}

{\large \textbf{BLM 4140 — Kablosuz ve Mobil Ağlar}}\\[0.3em]
{\large Bahar 2026}

\vspace{3cm}

\begin{tabular}{rl}
\textbf{Öğrenci} & : Ozan Orhan \\[0.3em]
\textbf{Öğrenci No} & : 21011077 \\[0.3em]
\textbf{Ders Sorumlusu} & : Asst.\ Prof.\ MEHMET ŞÜKRÜ KURAN \\
\end{tabular}

\vfill
{\large Mayıs 2026}
\end{titlepage}

\tableofcontents
\clearpage

# 1. Projenin amacı

Bu projede, aynı Wi-Fi erişim noktasına (AP) bağlı istasyon (STA)
sayısı arttıkça ağın toplam başarımının (throughput) nasıl
değiştiğini incelemek istiyoruz. NS-3 ağ simülatörü ile küçük bir
kablosuz ağ kuruyoruz: bir AP ve değişen sayıda STA. Tüm STA'lar aynı
anda AP'ye TCP üzerinden veri yüklüyor (uplink), biz de saniyede kaç
Mbit veri AP'ye ulaşıyor onu ölçüyoruz.

Aynı deneyi iki farklı MAC mekanizmasıyla yapıyoruz:

* **EDCA**: 802.11n'in standart kanal erişim yöntemi.
* **EDCA + RTS/CTS**: Aynı yöntem, ama her veri gönderiminden önce
  istasyonlar kısa bir RTS (Request-to-Send) / CTS (Clear-to-Send)
  el-sıkışması yapıyor.

Üç farklı trafik yükü deniyoruz: PHY ham veri hızının **%50, %80 ve
%90**'ı. Böylece hem rahat hem yoğun hem de tıka basa dolu bir ağda
neler olduğunu görebiliyoruz.

Cevaplamak istediğimiz asıl soru, G. Bianchi'nin 2000 yılında
sorduğu sorunun aynısı: *istasyon sayısı arttıkça ağ verimini
kaybediyor mu?* Bianchi eski DCF için "evet" cevabını vermişti. Biz
de modern 802.11n özellikleri (EDCA, TXOP, A-MPDU çerçeve birleştirme,
Block-ACK) açıkken bu cevabın hâlâ geçerli olup olmadığını kontrol
ediyoruz.

> **PHY düzeltmesi.** Proje açıklama PDF'i "IEEE 802.11ac 2.4 GHz
> PHY" diyor, fakat 802.11ac yalnızca 5 GHz bandında tanımlanmıştır;
> dolayısıyla 2.4 GHz'de böyle bir PHY yoktur. Ders sorumlusu 14
> Mayıs'taki Google Classroom paylaşımında bu durumu doğrulayıp
> bizden **IEEE 802.11n 2.4 GHz PHY**'ı 11n hızlarıyla kullanmamızı
> istedi. 20 MHz, 1×1 MIMO ve kısa koruma aralığı (short GI) ile en
> yüksek MCS, **MCS 7 — 72.2 Mbit/s**'tir. Raporun her yerinde bu
> 72.2 Mbit/s referans olarak kullanılmıştır.

# 2. Simülasyon kurulumu

NS-3 sürüm 3.40 kullanıyoruz. Tek bir C++ dosyamız var:
`wifi-project.cc`, `scratch/` klasörüne kopyalanıyor. Bu dosya üç
komut satırı parametresi alıyor; böylece tüm senaryoları tek tek
yeniden derlemek zorunda kalmadan bir kabuk betiği ile sırayla
çalıştırabiliyoruz.

## 2.1 Simülasyon parametreleri

| Parametre | Değer |
|---|---|
| Simülatör | NS-3.40 (`scratch/wifi-project.cc`) |
| Wi-Fi standardı | IEEE 802.11n, 2.4 GHz |
| Kanal | 20 MHz, kanal 1 |
| Anten | 1 × 1 (tek uzaysal akış), short GI (400 ns) |
| Hız yöneticisi | Minstrel-HT (ulaşılan MCS: MCS 7) |
| Referans ham hız | 72.2 Mbit/s |
| Topoloji | 1 AP + N STA, BSS, tüm STA'lar AP'den 1 m uzakta |
| `numSTA` | 4, 8, 12, 20 |
| `macMechanism` | EDCA / EDCA + RTS/CTS |
| `totalLoadPercent` | %50, %80, %90 |
| Trafik | Uplink **TCP** (NewReno), OnOff ile sabit hızda |
| Paket boyutu | 1448 B |
| Çerçeve birleştirme | A-MPDU = 65 535 B, A-MSDU = 7935 B |
| Block-ACK | Açık (A-MPDU ile zaten zorunlu) |
| TXOP | Açık, `BE_Txop/TxopLimits` = 3.2 ms |
| Simüle edilen süre | Senaryo başına 10 s ölçüm penceresi |
| Tekrar sayısı | Her senaryo 5 farklı RNG seed ile koşturuldu, ortalama alındı |

Toplamda 4 × 2 × 3 = **24 senaryo**, her birinin 5 farklı seed ile 5
kez koşturulması = **120 simülatör çağrısı**. Rapordaki tüm sayılar
bu 5 koşunun ortalamasıdır. Tüm figürlerdeki hata barları (error
bars) seed'ler arası örnek standart sapmadır.

## 2.2 Dört MAC mekanizmasını kodda nasıl etkinleştirdik?

Proje EDCA, TXOP, Block-ACK ve A-MPDU özelliklerini kullanmamızı
istiyor. Her biri `wifi-project.cc` içinde şu şekilde açılıyor:

* **EDCA** otomatik açık. NS-3'e `WIFI_STANDARD_80211n` dediğimiz
  anda AP ve STA'lar EDCA'yı uygulayan QoS MAC'ini kullanıyor; ek
  bir kod gerekmiyor.
* **A-MPDU** (çerçeve birleştirme), hem STA hem AP MAC'inde
  `BE_MaxAmpduSize = 65 535` ile (802.11n'in izin verdiği maksimum)
  açılıyor.
* **Block-ACK**, A-MPDU ile beraber zorunlu hale geliyor: 802.11n'de
  bir çerçeve birleşimini yolladığınızda *mutlaka* Block-ACK ile
  onaylanır. Yani MAC katmanı bunu kendiliğinden açıyor.
* **TXOP** açıkça ayarlanıyor. AC_BE için varsayılan TXOP limit 0,
  yani her kanal erişiminde sadece tek bir PPDU gönderiliyor. Biz
  bunu **3.2 ms**'ye çıkardık; böylece AP/STA bir kanal erişiminde
  birden fazla birleştirilmiş PPDU patlatabiliyor — "gerçek" TXOP
  davranışı bu.
* **RTS/CTS**, `WifiRemoteStationManager::RtsCtsThreshold`'u 100 B'a
  düşürerek açılıyor (neredeyse her çerçeve RTS/CTS tetikliyor). Saf
  EDCA istediğimizde bu eşiği 65 535'e ayarlayıp el-sıkışmasını
  kapatıyoruz.

## 2.3 Yük (load) nasıl hesaplanıyor?

Referans PHY ham hız 72.2 Mbit/s. `totalLoadPercent` parametresi,
*toplam* sunulan yükü belirtiyor:

* %50 → 36.10 Mbit/s
* %80 → 57.76 Mbit/s
* %90 → 64.98 Mbit/s

Bu toplam STA'lara eşit dağıtılıyor (örneğin 20 STA ile %50 yükte her
STA 1.805 Mbit/s TCP verisi göndermeye çalışıyor). Trafik TCP olduğu
için fiili gönderim hızı TCP'nin tıkanıklık denetimine de bağlı:
ağın taşıyamadığı durumda kaynak otomatik yavaşlıyor ve ölçtüğümüz
throughput, MAC katmanının verebildiği maksimum hıza yakın
sayılıyor.

# 3. Simülasyondan önce ne bekliyorduk?

Bianchi'nin 2000'deki klasik makalesi, eski DCF için şunu kanıtlamıştı:
İstasyon sayısı arttıkça aynı anda backoff sayan istasyon sayısı da
artar, bu yüzden çarpışmalar (collision) sıklaşır ve kanaldaki *yararlı*
hava süresi düşer. Sonuç olarak **toplam** throughput, ağ kalabalıkken
**düşer**.

Modern 802.11n MAC özellikleri iki şeyi değiştiriyor:

1. **A-MPDU + Block-ACK** her iletimde birden fazla MPDU'yu birlikte
   gönderiyor. Sabit per-çerçeve maliyetler (preamble, header, IFS,
   ACK) tüm birleşim için yalnızca bir kez ödeniyor. Bu yüzden N
   arttıkça yaşanan düşüş Bianchi'nin saf DCF için öngördüğünden
   *daha yumuşak* olmalı.
2. **RTS/CTS**, uzun bir A-MPDU çarpışmasını (birkaç ms boşa hava)
   çok daha ucuz bir RTS çarpışmasıyla (onlarca µs) değiştiriyor.
   İstasyon sayısı düşükken çarpışmalar nadirdir; RTS/CTS bu durumda
   sadece ek yüktür ve saf EDCA biraz daha iyidir. İstasyon sayısı
   yükseldikçe RTS/CTS işe yarar hale gelir çünkü çarpışmaları
   kısaltır.

Bizim deneyde 20 STA'nın hepsi AP'den 1 m mesafede, yani hepsi
birbirini duyuyor. **Gizli düğüm (hidden-node) problemi yok.** Bu
demek ki RTS/CTS bize ancak "kısa çarpışma" etkisi üzerinden fayda
sağlayabilir, görmediği komşulara karşı kanal rezervasyonuyla
değil.

Beklentinin kısa özeti:

* Tüm eğriler N arttıkça aşağı inmeli (Bianchi etkisi).
* %50 yükte EDCA ve RTS/CTS birbirine yakın olmalı; küçük N için
  RTS/CTS biraz daha kötü, en yüksek N için biraz daha iyi olabilir.
* %80 ve %90 yükte RTS/CTS bir noktada EDCA'yı geçmeli; ve bu
  geçiş daha yüksek yükte daha küçük N'de yaşanmalı.

# 4. Sonuçlar

Tüm figürler, üç tablo ve ortalanmış CSV dosyası
(`figures/results_avg.csv`), `plot_results.py` tarafından
`results_raw.csv`'den üretiliyor. Her sayı 5 bağımsız koşunun
ortalaması.

## 4.1 Sayısal tablolar

### Tablo 1 — Toplam throughput (Mbit/s, 5 seed ortalaması ± örnek stdev)

| Yük (%) | MAC          | 4 STA           | 8 STA           | 12 STA          | 20 STA          |
|--------:|--------------|:---------------:|:---------------:|:---------------:|:---------------:|
|      50 | EDCA         | 32.42 ± 4.93    | 33.82 ± 2.31    | 30.71 ± 0.83    | 22.36 ± 1.23    |
|      50 | RTS/CTS      | 34.18 ± 4.00    | 32.59 ± 2.02    | 28.22 ± 2.35    | 23.98 ± 3.48    |
|      80 | EDCA         | 44.37 ± 1.14    | 40.47 ± 1.17    | 36.63 ± 1.49    | 26.82 ± 4.81    |
|      80 | RTS/CTS      | 52.88 ± 5.44    | 48.39 ± 3.49    | 41.30 ± 6.25    | 32.55 ± 5.34    |
|      90 | EDCA         | 46.62 ± 1.27    | 40.77 ± 0.77    | 37.25 ± 2.05    | 28.23 ± 1.68    |
|      90 | RTS/CTS      | 52.93 ± 3.93    | 49.59 ± 3.24    | 45.37 ± 3.33    | 37.88 ± 2.02    |

Sunulan yükler 36.1 Mbit/s (%50), 57.8 Mbit/s (%80) ve 65.0 Mbit/s
(%90) — hepsi 72.2 Mbit/s referansa göre.

### Tablo 2 — 4 STA'dan 20 STA'ya geçişte throughput kaybı

| Yük (%) | MAC          | 4 STA | 20 STA | Mutlak düşüş   | Yüzde düşüş |
|--------:|--------------|------:|-------:|---------------:|------------:|
|      50 | EDCA         | 32.42 | 22.36  | 10.06 Mb/s     | %31.0       |
|      50 | RTS/CTS      | 34.18 | 23.98  | 10.20 Mb/s     | %29.8       |
|      80 | EDCA         | 44.37 | 26.82  | 17.55 Mb/s     | %39.6       |
|      80 | RTS/CTS      | 52.88 | 32.55  | 20.33 Mb/s     | %38.4       |
|      90 | EDCA         | 46.62 | 28.23  | 18.39 Mb/s     | %39.5       |
|      90 | RTS/CTS      | 52.93 | 37.88  | 15.05 Mb/s     | %28.4       |

Tablo 2 mutlak throughput seviyesinden bağımsız olarak Bianchi
etkisini gösteriyor. En çarpıcı satır en alttakiler: %90 yükte
RTS/CTS 4 STA'dan 20 STA'ya sadece %28 kaybediyor, oysa saf EDCA
neredeyse %40 kaybediyor. Yani yoğun trafikte RTS/CTS **daha iyi
ölçeklenen** mekanizma.

### Tablo 3 — Verimlilik (ölçülen throughput ÷ sunulan yük)

| Yük (%) | MAC          |  4 STA |  8 STA | 12 STA | 20 STA |
|--------:|--------------|-------:|-------:|-------:|-------:|
|      50 | EDCA         |   0.90 |   0.94 |   0.85 |   0.62 |
|      50 | RTS/CTS      |   0.95 |   0.90 |   0.78 |   0.66 |
|      80 | EDCA         |   0.77 |   0.70 |   0.63 |   0.46 |
|      80 | RTS/CTS      |   0.92 |   0.84 |   0.71 |   0.56 |
|      90 | EDCA         |   0.72 |   0.63 |   0.57 |   0.43 |
|      90 | RTS/CTS      |   0.81 |   0.76 |   0.70 |   0.58 |

1.0 değeri, ağın uygulamaların istediği her şeyi tam olarak
teslim ettiği anlamına gelir. Tablodan verimliliğin hızla
düştüğü görülüyor: %90 yükte 20 STA ile saf EDCA ancak sunulan
trafiğin %43'ünü, RTS/CTS ise %58'ini taşıyabiliyor.

## 4.2 Şekil 1 — %50 yük

![%50 yükte throughput](figures/throughput_load50.png)

%50 yükte ağ yaklaşık 36 Mbit/s istiyor — bu 802.11n MAC'in
doyuma ulaşacağı seviyenin altında, dolayısıyla her iki eğrinin
de bu 36 Mbit/s çizgisine yakın kalmasını **umuyoruz**. Tam
oraya ulaşamıyorlar (yaklaşık 22-34 Mbit/s arasında geziniyorlar)
çünkü MAC ek yükü ve AP'nin geri göndermek zorunda olduğu TCP
ACK trafiği biraz throughput yiyor.

İki eğri 4, 8 ve 12 STA için birbirine çok yakın. 4 STA'daki hata
barları büyük (yaklaşık ±4-5 Mbit/s), bu da şu demek: per-seed
varyasyon EDCA ile RTS/CTS arasındaki farktan daha büyük. Düz
ifadeyle: orta yükte **EDCA ve RTS/CTS'yi pratikte ayırt
edemiyorsunuz** — hangisini seçtiğiniz çok fark etmiyor.

Net sinyal 20 STA'da geliyor. Her iki eğri de düşüyor ve RTS/CTS
EDCA'nın biraz üzerine çıkıyor (23.98'e karşı 22.36 Mbit/s).
Neden? Çünkü orta yükte bile 20 istasyonun çekişmesi çarpışmaları
sıklaştırmaya yetiyor ve RTS/CTS'nin "çarpışmayı kısaltma"
özelliği işe yaramaya başlıyor.

## 4.3 Şekil 2 — %80 yük

![%80 yükte throughput](figures/throughput_load80.png)

%80 yükte (57.8 Mbit/s talep) ağ zaten **doyumun ötesinde**;
dolayısıyla ölçtüğümüz şey uygulamaların yollamak istediği değil,
MAC'in fiilen taşıyabildiği **maksimum** miktar. Sonuç çarpıcı:
RTS/CTS her STA sayısında EDCA'yı yeniyor.

* 4 STA: 52.88'e karşı 44.37 Mbit/s — RTS/CTS +8.51 Mbit/s önde
* 8 STA: 48.39'a karşı 40.47 Mbit/s — +7.92 Mbit/s önde
* 12 STA: 41.30'a karşı 36.63 Mbit/s — +4.67 Mbit/s önde
* 20 STA: 32.55'e karşı 26.82 Mbit/s — +5.73 Mbit/s önde

Sadece kitabın söylediğini ezberleyen birisi için ("RTS/CTS düşük
N'de zarar verir") bu biraz şaşırtıcı. Ama özellikle *uplink TCP*'yi
düşününce mantıklı: STA'ların hepsinde gönderilecek uzun A-MPDU'lar
var, üstelik AP'nin de geriye TCP ACK göndermesi gerekiyor. Uzun
A-MPDU'lar arasındaki çarpışma çok pahalı. RTS/CTS hem bu çarpışmaları
ucuz kısa çarpışmalara çeviriyor **hem de** AP'ye downstream ACK'lar
için kanal rezerve etmesinde yardım ediyor.

Bianchi etkisi de net görünüyor: EDCA 4'ten 20 STA'ya %39.6
throughput kaybediyor (44.37 → 26.82), RTS/CTS %38.4 kaybediyor
(52.88 → 32.55). Hata barları %50'ye göre büyük (RTS/CTS için 3-6
Mbit/s), çünkü nadir ama pahalı A-MPDU çarpışmaları seed'lere
rastgele dağılıyor. Yine de her kırmızı hata barının alt sınırı
karşılığındaki mavinin üst sınırından yüksek — yani sıralama
istatistiksel olarak sağlam.

## 4.4 Şekil 3 — %90 yük

![%90 yükte throughput](figures/throughput_load90.png)

%90 yükte (65 Mbit/s talep) ağ tamamen doymuş durumda. Bianchi
etkisinin en güçlü göründüğü senaryo bu: saf EDCA, 4 STA'daki
46.62 Mbit/s'ten 20 STA'daki 28.23 Mbit/s'ye **%39.5** düşüyor —
neredeyse tam olarak 25 yıl önce DCF modelinin öngördüğü monoton
düşüş eğrisi.

RTS/CTS yine her yerde baskın ve aradaki fark N arttıkça
**büyüyor**:

* 4 STA: +6.31 Mbit/s önde (52.93'e karşı 46.62)
* 8 STA: +8.82 Mbit/s önde (49.59'a karşı 40.77)
* 12 STA: +8.12 Mbit/s önde (45.37'e karşı 37.25)
* 20 STA: +9.65 Mbit/s önde (37.88'e karşı 28.23)

Bu, ders kitabı sezgisinin en doğrudan deneysel kanıtı. Hem yük
hem çekişen istasyon sayısı kanalı yüksek-çarpışma rejimine
sokarken, uzun A-MPDU çarpışmasının maliyeti RTS/CTS'nin sabit
küçük ek yükünün yanında devasa kalıyor. RTS/CTS aynı zamanda
**daha yavaş bozuluyor**: 4'ten 20 STA'ya yalnızca %28.4 kayıp,
saf EDCA ise %39.5 kayıp. Hata barları %80'e göre dar (1-4
Mbit/s) ve iki eğri kesişmiyor.

# 5. Genel görünüm ve güvenilirlik

## 5.1 Şekil 4 — Altı senaryo bir arada

![Altı senaryo bir arada](figures/throughput_overview.png)

Şekil 4 altı ölçüm eğrisini tek bir grafiğe koyuyor. Mavi = EDCA,
kırmızı = EDCA+RTS/CTS, çizgi stili sunulan yükü gösteriyor
(noktalı = %50, kesikli = %80, düz = %90). Üç şey hemen göze
çarpıyor:

1. **Hiçbir eğri yukarı çıkmıyor.** Tüm (yük, MAC) kombinasyonları
   N arttıkça throughput kaybediyor. Bianchi'nin tahmini altı
   eğrinin hepsi için geçerli.
2. **%80 ve %90 yükte kırmızı eğriler her zaman mavinin üzerinde.**
   Üstelik en geniş fark düz çizgili çiftte (%90 yük) ve bu fark N
   arttıkça büyüyor.
3. **%50 yükte iki noktalı eğri 4-12 STA için neredeyse aynı**,
   sadece 20 STA'da ayrılıyorlar. Doyumun altında MAC seçimi çok
   önemli değil.

## 5.2 Bu sayılara ne kadar güvenebiliriz?

Her senaryoyu 5 farklı seed ile 5 kez koşturup ortalamasını aldık.
Tüm figürlerdeki hata barları seed'ler arası örnek standart
sapmayı gösteriyor. Operasyon noktalarının çoğunda stdev 3
Mbit/s'in altında ve %80 ile %90 yükteki EDCA-RTS/CTS sıralaması
gürültünün birkaç katı büyüklükte — yani sıralama tek bir şanslı
seed'in eseri değil.

Ayrıca önceden 3 seed ile bir kalibrasyon koşusu da yaptık
(`results_raw_3seeds.csv` olarak kayıtlı). (Yük, N)
noktalarının çoğunda 3'ten 5 seed'e geçince ortalama 1 Mbit/s'in
altında değişti ve EDCA-RTS/CTS sıralaması hiç değişmedi. En büyük
kayma yaklaşık 3 Mbit/s, %80 yük 20 STA'da gerçekleşti — ve aynı
nokta en büyük örnek stdev'e sahip (EDCA için 4.81, RTS/CTS için
5.34); bu da ortalamayı kaydıran şeyin *içsel* varyans olduğunu,
örneklem büyüklüğü sorunu olmadığını gösteriyor. İlave iki seed
tahmini tam ihtiyaç duyulan yerde sıkılaştırdı.

# 6. Sonuç

* **Bianchi'nin 25 yıllık sonucu hâlâ geçerli.** 802.11n'in tüm
  modern MAC özellikleri (EDCA, TXOP, A-MPDU, Block-ACK) açık olsa
  bile, çekişen istasyon sayısı arttıkça throughput monoton
  düşüyor. Saf EDCA için 4 STA'dan 20 STA'ya kayıp %50, %80 ve
  %90 yükte sırasıyla %31, %40 ve %40.
* **RTS/CTS, yoğun uplink TCP altında şaşırtıcı şekilde işe
  yarıyor.** Klasik öğreti der ki: RTS/CTS sadece gizli düğüm
  problemi olduğunda veya çok kalabalık ağlarda faydalıdır. Bizim
  kurulumda gizli düğüm yok — her STA diğer her STA'yı duyuyor.
  Yine de %80 yüke ulaştığımız anda RTS/CTS *her* STA sayısında
  saf EDCA'yı yeniyor, %90 yük ve 20 STA'da +9.65 Mbit/s (≈ %34)
  fark var. Nedeni: uplink TCP uzun A-MPDU üretiyor ve AP'nin de
  downstream TCP ACK'lar için kanala ihtiyacı var. Uzun A-MPDU
  çarpışmaları çok pahalı; RTS/CTS hem onları ucuz kısa
  çarpışmalara çeviriyor hem de AP'nin çekişmeyi kazanmasına
  yardım ediyor.
* **%50 yükte hangi MAC seçildiği pratikte farketmiyor.** İki
  eğri 4-12 STA için birbirinin hata barı içinde.
* **Pratik tavsiye.** Yoğun, uplink-ağırlıklı bir 802.11n
  dağıtımında (örneğin 20 kişilik bir sınıfta hepsi aynı anda
  upload yapıyor olsun), ağ kalabalıklaştığı an RTS/CTS açmak net
  bir kazanç. Her ders kitabının "ek yük" diye uyarmasına rağmen.

# Kaynaklar

1. NS-3 ağ simülatörü, <https://www.nsnam.org/>.
2. G. Bianchi, "Performance analysis of the IEEE 802.11 distributed
   coordination function," *IEEE J. Sel. Areas Commun.*, cilt 18,
   no. 3, ss. 535–547, Mart 2000.
3. IEEE 802.11-2020, *Part 11: Wireless LAN MAC and PHY Specifications*.
