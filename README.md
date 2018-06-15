OS非依存プログラム ACT.2
========================

？？？「BIOSなんてもう古い！これからはUEFIの時代だ！」

「普通の」パソコンをターゲットにした、OSに依存しないプログラムです。
BIOSではなく、(U)EFIに起動してもらいます。

有用な知見
----------

* プログラムはexeファイルと同じPEフォーマットで、Subsystemを0x0Aにするといいらしい。
* プログラムは32ビットモードで起動されるらしい。 (32ビットのPEフォーマットファイルだから?)
* プログラムをデイスクの `/EFI/BOOT/BOOTIA32.EFI` に保存しておくと、自動起動してもらえるらしい。
