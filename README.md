# RF_Project – Headless Mod: Tanımladık Ama Dinletemedik 😤🧪

## Türkçe

Bu proje, `#define HEADLESS` satırının aslında bir illüzyon olduğunu acı ama komik bir şekilde fark ettiğimiz bir firmware laboratuvarıdır.

### Gerçekler:
- Headless mod diye bir şey varmış gibi davranıyor ama aslında yok.
- `#define HEADLESS` tanımladık, loglar “headless moddayım” dedi, ama sistem “ben kendi bildiğimi okurum” dedi.
- ISR erişimi, buffer davranışı, dummy loglar... hepsi kafasına göre takılıyor.

> Geliştiriciye sesleniyoruz:
> Headless mod sadece bir #define değil, bir yaşam biçimi olmalıydı. Ama şu an sadece bir etiket.
> Tanımlıyoruz, ama sistem “tanıdım ama tanımamış gibi yapıyorum” diyor.

### Hedefimiz:
- DummyWire ve WireWrapper ile gerçekten headless çalışan bir test altyapısı kurmak
- Loglardan sistemin ruh halini anlamak
- Kodun “headless” değil “headstrong” olduğunu belgelemek

🧪 Bonus: Eğer `#define HEADLESS` seni kandırdıysa, yalnız değilsin. Biz de bir hafta boyunca kandırıldık.
🧪 Gerçeklik Kontrolü: HEADLESS modu bir mod değildir. Bir ruh halidir. Biz tanımladık. Sistem bunu görmezden geldi. Testlerimiz şimdi bu ihaneti doğruluyor.
🏗️ Yapım aşamasında. İndirip de başınızı belaya sokmayın....

---

## English

This project is a firmware lab built on the painful realization that `#define HEADLESS` is more of a costume than a command.

### The truth:
- Headless mode pretends to exist, but doesn’t actually behave like it.
- We defined `HEADLESS`, the logs said “I’m headless,” but the system said “I do what I want.”
- ISR access, buffer behavior, dummy logs... all went rogue.

> A message to the developers:
> Headless mode should be a lifestyle, not a label.
> We define it, but the system replies “I acknowledge your define... and ignore it.”

### Our mission:
- Build a truly headless test infrastructure with DummyWire and WireWrapper
- Read logs like mood rings
- Document how the code is not headless, but headstrong

---

🧪 Bonus: If #define HEADLESS tricked you into believing you were in control, welcome to the club. We spent a whole week chasing a mirage.
🧪 Reality Check: HEADLESS mode is not a mode. It’s a mood. We defined it. The system ignored it. Our tests now verify the betrayal.
🏗️ Under construction. Don't download it and get yourself into trouble...
