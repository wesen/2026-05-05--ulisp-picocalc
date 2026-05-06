1

2

3

4

5

6

7

8

9

10

11

12

13

14

15

16

17

18

19

20

21

22

23

24

25

26

27

28

29

30

31

32

33

34

35

36

37

38

39

40

41

42

43

44

45

46

47

48

49

50

51

52

53

54

55

56

57

58

59

60

61

62

63

64

65

66

67

68

69

70

71

72

73

74

75

76

77

78

79

80

81

82

83

84

85

86

87

88

89

90

91

92

93

94

95

96

97

98

99

100

101

102

103

104

105

106

107

108

109

110

111

112

113

114

115

116

117

118

119

120

121

122

123

124

125

126

127

128

129

130

131

132

133

134

135

136

137

138

139

140

141

142

143

144

145

146

147

148

149

150

151

152

153

154

155

156

157

158

159

160

161

162

163

164

165

166

167

168

169

170

171

172

173

174

175

176

177

178

179

180

181

182

183

184

185

186

187

188

189

190

191

192

193

194

195

196

197

198

199

200

201

202

203

204

205

206

207

208

209

210

211

212

213

214

215

216

217

218

219

220

221

222

223

224

225

226

227

228

229

230

231

232

233

234

235

236

237

238

239

240

241

242

243

244

245

246

247

248

249

250

251

252

253

254

255

256

257

258

259

260

261

262

263

264

265

266

267

268

269

270

271

272

273

274

275

276

277

278

279

280

281

282

283

284

285

286

287

288

289

290

291

292

293

294

295

296

297

298

299

300

301

302

303

304

305

306

307

308

309

310

311

312

313

314

315

316

317

318

319

320

321

322

323

324

325

326

327

328

329

330

331

332

333

334

335

336

337

338

339

340

341

342

343

344

345

346

347

348

349

350

351

352

353

354

355

356

357

358

359

360

361

362

363

364

365

366

367

368

369

370

371

372

373

374

375

376

377

378

379

380

381

382

383

384

385

386

387

388

389

390

391

392

393

394

395

396

397

398

399

400

401

402

403

404

405

406

407

408

409

410

411

412

413

414

415

416

417

418

419

420

421

422

423

424

425

426

427

428

429

430

431

432

433

434

435

436

437

438

439

440

441

442

443

444

445

446

447

448

/\*\*

\* PicoCalc SD Firmware Loader

\*

\* Author: Hsuan Han Lai

\* Email: hsuan.han.lai@gmail.com

\* Website: https://hsuanhanlai.com

\* Year: 2025

\*

\*

\* This project is a bootloader for the PicoCalc device, designed to load and execute

\* firmware applications from an SD card.

\*

\*/

#include <stdio.h>

#include <string.h>

#include "pico/stdlib.h"

#include "pico/bootrom.h"

#include "pico/usb\_reset\_interface.h"

#include "hardware/gpio.h"

#include "hardware/clocks.h"

#include "debug.h"

#include "i2ckbd.h"

#include "lcdspi.h"

#include <hardware/flash.h>

#include <errno.h>

#include <hardware/watchdog.h>

#include "config.h"

#include "blockdevice/sd.h"

#include "filesystem/fat.h"

#include "filesystem/vfs.h"

#include "text\_directory\_ui.h"

#include "key\_event.h"

const uint LEDPIN = 25;

// Vector and RAM offset

#if PICO\_RP2040

#define VTOR\_OFFSET M0PLUS\_VTOR\_OFFSET

#define MAX\_RAM 0x20040000

#elif PICO\_RP2350

#define VTOR\_OFFSET M33\_VTOR\_OFFSET

#define MAX\_RAM 0x20080000

#endif

uint8\_t status\_flag;//0 no sdcard,1 has sd card

bool sd\_card\_inserted(void)

{

status\_flag =!gpio\_get(SD\_DET\_PIN);

// Active low detection - returns true when pin is low

return (bool)status\_flag;

}

bool fs\_init(void)

{

DEBUG\_PRINT("fs init SD\\n");

blockdevice\_t \*sd = blockdevice\_sd\_create(spi0,

SD\_MOSI\_PIN,

SD\_MISO\_PIN,

SD\_SCLK\_PIN,

SD\_CS\_PIN,

125000000 / 2 / 4, // 15.6MHz

true);

filesystem\_t \*fat = filesystem\_fat\_create();

int err = fs\_mount("/", fat, sd);

if (err!= -1)

{

DEBUG\_PRINT("Mounted SD card at /\\n");

return true;

}

err = fs\_format(fat, sd);

if (err == -1)

{

DEBUG\_PRINT("Failed to format SD card\\n");

return false;

}

err = fs\_mount("/", fat, sd);

if (err == -1)

{

DEBUG\_PRINT("Failed to mount SD card at /\\n");

return false;

}

DEBUG\_PRINT("Mounted SD card at /\\n");

return true;

}

static bool \_\_not\_in\_flash\_func(is\_same\_existing\_program)(FILE \*fp)

{

uint8\_t buffer\[FLASH\_SECTOR\_SIZE\] = {0};

size\_t program\_size = 0;

size\_t len = 0;

while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)

{

uint8\_t \*flash = (uint8\_t \*)(XIP\_BASE + SD\_BOOT\_FLASH\_OFFSET + program\_size);

if (memcmp(buffer, flash, len)!= 0)

return false;

program\_size += len;

}

return true;

}

// Check if a valid application exists in flash by examining the vector table

static bool is\_valid\_application(uint32\_t \*app\_location)

{

// Check that the initial stack pointer is within a plausible RAM region.

// Assumed range for Pico: 0x20000000 to 0x20040000 + SCRATCH\_X + SCRATCH\_Y

// Which is the same as the range 0x20000000 to 0x20042000

uint32\_t stack\_pointer = app\_location\[0\];

if (stack\_pointer < 0x20000000 || stack\_pointer > MAX\_RAM + 2\*4\*1024) // MAX\_RAM + 8KB (4KB per scratch region)

{

return false;

}

// Check that the reset vector is within the valid flash application area

uint32\_t reset\_vector = app\_location\[1\];

if (reset\_vector < (0x10000000 + SD\_BOOT\_FLASH\_OFFSET) || reset\_vector > (0x10000000 + PICO\_FLASH\_SIZE\_BYTES))

{

return false;

}

return true;

}

// This function must run from RAM since it erases and programs flash memory

static bool \_\_not\_in\_flash\_func(load\_program)(const char \*filename)

{

FILE \*fp = fopen(filename, "r");

if (fp == NULL)

{

DEBUG\_PRINT("open %s fail: %s\\n", filename, strerror(errno));

return false;

}

// Check file size to ensure it doesn't exceed the available flash space

if (fseek(fp, 0, SEEK\_END)!= 0)

{

DEBUG\_PRINT("seek err: %s\\n", strerror(errno));

fclose(fp);

return false;

}

long file\_size = ftell(fp);

if (file\_size <= 0) // Negative, to include error code -1

{

DEBUG\_PRINT("invalid size: %ld\\n", file\_size);

fclose(fp);

return false;

}

if (file\_size > MAX\_APP\_SIZE)

{

DEBUG\_PRINT("file too large: %ld > %d\\n", file\_size, MAX\_APP\_SIZE);

fclose(fp);

return false;

}

DEBUG\_PRINT("updating: %ld bytes\\n", file\_size);

if (fseek(fp, 0, SEEK\_SET)!= 0)

{

DEBUG\_PRINT("seek err: %s\\n", strerror(errno));

fclose(fp);

return false;

}

// Only check for validity after the guard clauses to make sure the file pointer (fp) is valid

if ( is\_same\_existing\_program(fp) && is\_valid\_application((uint32\_t\*)(XIP\_BASE + SD\_BOOT\_FLASH\_OFFSET)) )

{

DEBUG\_PRINT("Same program already valid in flash, skipping\\n");

fclose(fp);

return true;

}

size\_t program\_size = 0;

uint8\_t buffer\[FLASH\_SECTOR\_SIZE\] = {0};

size\_t len = 0;

// Erase and program flash in FLASH\_SECTOR\_SIZE chunks

while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)

{

// Ensure we don't write beyond the application area

if ((program\_size + len) > MAX\_APP\_SIZE)

{

DEBUG\_PRINT("err: write beyond app area\\n");

fclose(fp);

return false;

}

uint32\_t ints = save\_and\_disable\_interrupts();

flash\_range\_erase(SD\_BOOT\_FLASH\_OFFSET + program\_size, FLASH\_SECTOR\_SIZE);

flash\_range\_program(SD\_BOOT\_FLASH\_OFFSET + program\_size, buffer, len);

restore\_interrupts(ints);

program\_size += len;

}

DEBUG\_PRINT("program loaded\\n");

fclose(fp);

return true;

}

// This function jumps to the application entry point

// It must update the vector table and stack pointer before jumping

void \_\_not\_in\_flash\_func(launch\_application\_from)(uint32\_t \*app\_location)

{

// https://vanhunteradams.com/Pico/Bootloader/Bootloader.html

uint32\_t \*new\_vector\_table = app\_location;

volatile uint32\_t \*vtor = (uint32\_t \*)(PPB\_BASE + VTOR\_OFFSET);

\*vtor = (uint32\_t)new\_vector\_table;

asm volatile(

"msr msp, %0\\n"

"bx %1\\n"

:

: "r"(new\_vector\_table\[0\]), "r"(new\_vector\_table\[1\])

:);

}

void boot\_default()

{

DEBUG\_PRINT("entering boot\_default\\n");

// Get the pointer to the application flash area

uint32\_t \*app\_location = (uint32\_t \*)(XIP\_BASE + SD\_BOOT\_FLASH\_OFFSET);

launch\_application\_from(app\_location);

// We should never reach here

while (1)

{

tight\_loop\_contents();

}

}

void boot\_fwupdate()

{

DEBUG\_PRINT("entering boot\_fwupdate\\n");

lcd\_init();

lcd\_clear();

draw\_rect\_spi(20, 140, 300, 180, WHITE);

lcd\_set\_cursor(30, 150);

lcd\_print\_string\_color((char \*)"FIRMWARE UPDATE", BLACK, WHITE);

sleep\_ms(2000);

uint gpio\_mask = 0u;

reset\_usb\_boot(gpio\_mask, PICO\_STDIO\_USB\_RESET\_BOOTSEL\_INTERFACE\_DISABLE\_MASK);

}

int load\_firmware\_by\_path(const char \*path)

{

text\_directory\_ui\_set\_status("STAT: Flashing firmware...");

// Attempt to load the application from the SD card

// bool load\_success = load\_program(FIRMWARE\_PATH);

bool load\_success=false;

// Get the pointer to the application flash area

uint32\_t \*app\_location = (uint32\_t \*)(XIP\_BASE + SD\_BOOT\_FLASH\_OFFSET);

// Check if there is an already valid application in flash

bool has\_valid\_app = is\_valid\_application(app\_location);

if(path == NULL) {

load\_success = true;

}else{

load\_success = load\_program(path);

}

if (load\_success || has\_valid\_app)

{

text\_directory\_ui\_set\_status("STAT: Launching app...");

DEBUG\_PRINT("launching app\\n");

// Small delay to allow printf to complete

sleep\_ms(100);

launch\_application\_from(app\_location);

}

else

{

text\_directory\_ui\_set\_status("ERR: No valid app");

DEBUG\_PRINT("no valid app, halting\\n");

sleep\_ms(2000);

// Trigger a watchdog reboot

watchdog\_reboot(0, 0, 0);

}

// We should never reach here

while (1)

{

tight\_loop\_contents();

}

}

void final\_selection\_callback(const char \*path)

{

char status\_message\[128\];

const char \*extension = ".bin";

if(path == NULL) {

//load default app from flash

snprintf(status\_message, sizeof(status\_message), "SEL: %s", "FLASH+200k");

text\_directory\_ui\_set\_status(status\_message);

sleep\_ms(200);

load\_firmware\_by\_path(path);

return;

}

// Trigger firmware loading with the selected path

DEBUG\_PRINT("selected: %s\\n", path);

size\_t path\_len = strlen(path);

size\_t ext\_len = strlen(extension);

if (path\_len < ext\_len || strcmp(path + path\_len - ext\_len, extension)!= 0)

{

DEBUG\_PRINT("not a bin: %s\\n", path);

snprintf(status\_message, sizeof(status\_message), "Err: FILE is not a.bin file");

text\_directory\_ui\_set\_status(status\_message);

return;

}

snprintf(status\_message, sizeof(status\_message), "SEL: %s", path);

text\_directory\_ui\_set\_status(status\_message);

sleep\_ms(200);

load\_firmware\_by\_path(path);

}

int read\_bootmode()

{

int key = keypad\_get\_key();

int \_x;

DEBUG\_PRINT("read\_bootmode key = %d\\n", key);

while((\_x = keypad\_get\_key()) > 0) {

// drain the keypad input buffer

DEBUG\_PRINT("read\_bootmode subsequent key = %d\\n", \_x);

}

int bootmode = 0; // Default boot mode

if (key == KEY\_ARROW\_UP)

{

bootmode = 1; // SD card boot mode

}

else if (key == KEY\_ARROW\_DOWN)

{

bootmode = 2; // Firmware update mode

}

return bootmode;

}

int main()

{

uint32\_t cur\_time,last\_time=0;

stdio\_init\_all();

uart\_init(uart0, 115200);

uart\_set\_format(uart0, 8, 1, UART\_PARITY\_NONE); // 8-N-1

uart\_set\_fifo\_enabled(uart0, false);

// Initialize SD card detection pin

gpio\_init(SD\_DET\_PIN);

gpio\_set\_dir(SD\_DET\_PIN, GPIO\_IN);

gpio\_pull\_up(SD\_DET\_PIN); // Enable pull-up resistor

keypad\_init();

// Check bootmode now: 0=default, 1=sdcard, 2=fwupdate

int bootmode = read\_bootmode();

DEBUG\_PRINT("bootmode = %d\\n", bootmode);

switch(bootmode) {

case 0:

// BOOTMODE\_DEFAULT

boot\_default();

break;

case 2:

// BOOTMODE\_FWUPDATE

boot\_fwupdate();

break;

case 1:

// BOOTMODE\_SDCARD

default:

break;

}

// BEGIN SDCARD BOOT

lcd\_init();

lcd\_clear();

text\_directory\_ui\_pre\_init();

cur\_time = time\_us\_64() / 1000;

last\_time = cur\_time;

// Check for SD card presence

DEBUG\_PRINT("Checking for SD card...\\n");

if (!sd\_card\_inserted())

{

DEBUG\_PRINT("SD card not detected\\n");

text\_directory\_ui\_set\_status("Enter to exec.");

text\_directory\_ui\_update\_header(1);

text\_directory\_ui\_update\_title();

// Poll until SD card is inserted

text\_directory\_ui\_draw\_default\_app();

text\_directory\_ui\_set\_final\_callback(final\_selection\_callback);

while (!sd\_card\_inserted())

{

cur\_time = time\_us\_64() / 1000;

int key = keypad\_get\_key();

if (key!= 0)

process\_key\_event(key);

sleep\_ms(20);

if(cur\_time - last\_time > BAT\_UPDATE\_MS) {

text\_directory\_ui\_update\_title();

last\_time = cur\_time;

}

}

// Card detected, wait for it to stabilize

DEBUG\_PRINT("SD card detected\\n");

text\_directory\_ui\_set\_status("SD card detected. Mounting...");

sleep\_ms(1500); // Wait for card to stabilize

}

else

{

// If SD card is detected at boot, wait for stabilization

DEBUG\_PRINT("SD card stabilization delay on boot\\n");

text\_directory\_ui\_set\_status("Stabilizing SD card...");

sleep\_ms(1500); // Delay to allow the SD card to fully power up and stabilize

}

// Initialize filesystem

if (!fs\_init())

{

text\_directory\_ui\_set\_status("Failed to mount SD card!");

DEBUG\_PRINT("Failed to mount SD card\\n");

sleep\_ms(2000);

watchdog\_reboot(0, 0, 0);

}

sleep\_ms(500);

lcd\_clear();

text\_directory\_ui\_init();

text\_directory\_ui\_set\_final\_callback(final\_selection\_callback);

while(keypad\_get\_key() > 0) {

// drain the keypad input buffer

}

text\_directory\_ui\_run();

}