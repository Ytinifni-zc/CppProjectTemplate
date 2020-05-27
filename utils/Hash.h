//
// Created by inFinity on 2019-09-09.
//

#ifndef GI_HASH_H
#define GI_HASH_H

#include "Type.h"

static const size_t rtab[256] = {
        0x38e93aa9b144bda2ul, 0x960917d2c2c1a2d0ul, 0xf987068e080f6339ul, 0x139e10904dcb1856ul,
        0x7029b32ef8239339ul, 0x0d6a94af0cfb711dul, 0xec0422c8cc82d12bul, 0x70392b5093b173a2ul,
        0x637f4a9d67c3c0a3ul, 0x09226c20ecc6885aul, 0xfe49eb4221257862ul, 0x2ea7c523885360bcul,
        0x8922bfdd63006621ul, 0x1b0315c2684a49a9ul, 0xc61b4dff533d206bul, 0xa4a7e43d0b89546eul,
        0xa390266affbb3837ul, 0x6d6886f58151a639ul, 0x2ee2672ca133cf9bul, 0x54cca6168348e870ul,
        0x02dfa91a9eaf386dul, 0x6167b087708dceeaul, 0x32ce1c94f6b8b629ul, 0x3711df620fab88a4ul,
        0x30df957f6c268277ul, 0x8d0811ad76a1765ful, 0x726cdf6dc3f62957ul, 0x08b122af8a124ba3ul,
        0xafecd2cf94c15335ul, 0xd85e198ec42019fcul, 0x318d432b821cce6eul, 0xdc66b013eac54b21ul,
        0x4da099b1d5b381eeul, 0xd0ec9887c7c159e8ul, 0xe643b3c100c5bd8eul, 0x11679c279eea9542ul,
        0x79307576c04587aeul, 0x1c3d6b927f5dcb24ul, 0x88a749f6f1028f81ul, 0x2e52e4ef7160f5bbul,
        0x7871571894d2390aul, 0xe29beda4c83c9f22ul, 0x05abfaa357eafd1bul, 0x41cce392870da74aul,
        0x54d54e9a89d25107ul, 0x8d885bec0eb940a4ul, 0x7e8dcd59db7c1c6bul, 0x6e544e221f45d52ful,
        0xcadad836ba5a6621ul, 0x2380667566cee47aul, 0x4c1950bb9d82c110ul, 0x9bd20af21a3cb1f5ul,
        0x446ba4097169e713ul, 0x927900c45d977949ul, 0x8c73f7ab277dc9c8ul, 0x1f276e404ed3feb1ul,
        0x41624f650031f6c7ul, 0xc3d6e42e585738e0ul, 0x31b23c2e74df754cul, 0x2af6c864c05b00f4ul,
        0xef8bec1ef5c7590ful, 0xf74402cf08a5a942ul, 0xec6ad8cc8597f50aul, 0x6adf839bd7199372ul,
        0x0ed0a820ae765b3aul, 0x782ea4816103f2f0ul, 0xad5c00e730737bf7ul, 0xeb1930ab40d6c44eul,
        0xd65706f13fa89b30ul, 0x525cc03bf4841bc0ul, 0x653dda543926d3e5ul, 0xd131996597bd2db3ul,
        0x1bb33a368ef16469ul, 0xc45894961ab0470cul, 0x8585109d0fb64906ul, 0xa1a7a52cfb6a0a65ul,
        0xe8c284f9e1fc3ca2ul, 0x4ff686fbfe3ce189ul, 0x589d846738ce23f8ul, 0x2ccc227b87af7a12ul,
        0xc98b627e12d4d12bul, 0xae3e79716fe577b7ul, 0xe481746f9105f438ul, 0x0231d7a24605f4d9ul,
        0xf6741586635cdb96ul, 0xb6b92cc84984cbccul, 0xf3c15be1bee6e1bdul, 0xa3e74eb443d5c2c5ul,
        0xcf64bd38c1f562ccul, 0xa5b55d41dae7b3c7ul, 0x23e4f9292db71b7aul, 0x7acefe7ff78726dbul,
        0xe3bad26d00139aedul, 0xbf117f0e55f66eeaul, 0xdb9fdd2cb7c8f54cul, 0xeff80c9d3fa7632ful,
        0xf65b20ed600a9f93ul, 0x9035286e23bbbb7bul, 0x95b4d683436a9960ul, 0x513ee19b6a998a61ul,
        0xc8d1bfa8efb7c349ul, 0xc2fe99159fc54f97ul, 0x2c9be8d78112eb8eul, 0xf1dfe85001df69acul,
        0xe1c5ffa79fdb7509ul, 0x0df86932fc8c065cul, 0xa5fd79b43e358bb6ul, 0x62e76a41497a1c83ul,
        0x497094311b616305ul, 0x42c0c69ceb7ced38ul, 0x4eb35222f6e67f4ful, 0x29fceb11c4ac91a9ul,
        0xd1a3157f5d439627ul, 0xc293c7efda956725ul, 0x8e9f34f743f822adul, 0xa2e9957e3fb6eaf5ul,
        0xb98c5ca900968076ul, 0x7e0d0617aadbb3e7ul, 0x7fff8303fd0da04cul, 0x1bf2358b93c9004bul,
        0xc8966a3137ab2552ul, 0x77e24d735ba78048ul, 0x7e3e8fe06e551bdcul, 0xa4e9f7669bba1340ul,
        0x31c759d0d1be0042ul, 0x43ea570762218186ul, 0x49d9b40a84cd0707ul, 0xbcf962b0be3eb7aeul,
        0xa5813cfbcfb67040ul, 0x95dfba755ee6b89bul, 0x2a4fa918c541925bul, 0x562b5d3c7e2c36e7ul,
        0xec5da7d3f5c3d3c4ul, 0x8672c470cf03ef22ul, 0x9a7b152ed20a4d57ul, 0x512483e9f32ca9faul,
        0x01bdac76a9a83024ul, 0x68f13c4a3593461bul, 0xd45e43ff6633b183ul, 0x0771997d58073d02ul,
        0x6809f97e5c4d5af1ul, 0x406ee033ec764cccul, 0x9f155e0da508644bul, 0xc80087adb53c0c40ul,
        0xd1828201df399cccul, 0x22a0c79eaeeaaab0ul, 0x0d1ee8880344cfc5ul, 0x1158b70562df6398ul,
        0x6089decb95dc42e9ul, 0xd1aa0766a92ae1bbul, 0x6c9ee809eb5d8ff7ul, 0xb5dbf3ba51ae77d8ul,
        0x040a72e576c14cf1ul, 0x08b34ff0da600641ul, 0x492d67c654e0e3feul, 0x3904510f382cf861ul,
        0xf1706c1b84dc18dbul, 0x9143f04bba3152fdul, 0x56ad7d08f4ab1368ul, 0x2000749a91a52590ul,
        0x931119da85e712e0ul, 0x829515339365b6b4ul, 0x83082cd79488ef54ul, 0x56778d20b1e52a17ul,
        0x473dc0f8506588aaul, 0x8aed7294826e99dful, 0xe368b026a1161fd3ul, 0xac22bde51e9c9678ul,
        0x5283cdeb992f88fbul, 0x1f03b5cf0b665bf6ul, 0xb9cfc98f9d80fd8eul, 0x26e96b92a31f1bdaul,
        0xe60cdcf9b95dd9b8ul, 0xfb9d45f3c10232a8ul, 0x03e126aece5b8589ul, 0xabc379d14db4d02bul,
        0xb041e868bf6b53fcul, 0x352ea8a6d9b0eca4ul, 0x29a6c6c0e724fdaeul, 0x92ed261b36adc3b0ul,
        0x54be8675584422dbul, 0x68c89a384c9c24c8ul, 0x5834f6128762eeacul, 0xa2c5375bb331e615ul,
        0x6015479900f95612ul, 0x717c149b20bfac9cul, 0x15a1e6402c33973eul, 0xd118363e55fc0048ul,
        0x5d845d5a14bb5a02ul, 0xc59a95e131e29198ul, 0xdf3d445a0ec6b6b2ul, 0xe9270f18901005aaul,
        0xbe4ac8de40c727d5ul, 0x5f855aced267ff1cul, 0x419322d4ce78d691ul, 0x36015b2528ea84c5ul,
        0xf54b3c254a8d8d04ul, 0xfbe410bb705002aeul, 0x07bac68efe19f88eul, 0x33df9819364de4feul,
        0x3cea137358eec043ul, 0xddc140cf05945cfful, 0x345306415edb5078ul, 0x5cb2c84ece3977bdul,
        0x2cd631d264603b50ul, 0x8aa341a2b01516edul, 0xaf6b399e76c4418dul, 0x9574f9300ce45b29ul,
        0x99c41faffff9e225ul, 0x437f809a88fa4269ul, 0xf7f0b15315a7d86aul, 0x6122494b15625cd9ul,
        0x5124b9014ee206acul, 0x03bab5c28d52f402ul, 0x6bdf24cbfb2b1b8cul, 0x8f353f5f9444b860ul,
        0x149719d8870b2a2bul, 0xf741ebea8eed7818ul, 0xd35dbc8bbfd1f7caul, 0xfde505df6dd07467ul,
        0x6125306da4d6a38dul, 0xd2af38affd1070aeul, 0x1ffbeae984c02c28ul, 0x12f6a12f7a61ade8ul,
        0x3992ce66b7df9f29ul, 0x384cad3908b5a0deul, 0x3052b7df000026b8ul, 0xf8a45b1b04d0f2a1ul,
        0x01fa54c51075a7f7ul, 0x4cfce249a78d0626ul, 0x1b2e44a84ba48447ul, 0xd26055fc72b5e89cul,
        0x42798c82c350719eul, 0x8c0c4e12ea71e78cul, 0x2aabe281c469e1aaul, 0xf2d2f3b33a03ad6eul,
        0xedbee13ce3ca0c56ul, 0x74fa1e0d832f10baul, 0x4a5bedc230688905ul, 0x1d3fafc325d91409ul,
        0x13469dfdd60fefa9ul, 0xd3f1d378a3015fd8ul, 0xcff290fc3a2d3ab3ul, 0xbc71a192e588eb8ful,
        0xaf235696c37f4fd3ul, 0xb49cadc4d3c4071aul, 0xec4b298bbc19d767ul, 0x96067f2a06fb177ful,
        0xc241834108228b43ul, 0x74773f423bf50de0ul, 0x9e6273f155169800ul, 0x8d5b89dda7e7b710ul,
        0xbafa2018d12d8c41ul, 0xb81775d216054151ul, 0x18a8651a3002332eul, 0x8f16730042f14fd1ul,
};

void hashUpdate(size_t &h, UInt64 i) {
    i++;
    while (i > 0) {
        h ^= rtab[i & 0xff];
        const size_t b = (h & 0x8000000000000000ul) >> 63;
        i = i >> 8;
        h = (h << 1) | b;
    }
}

#endif //GI_HASH_H
