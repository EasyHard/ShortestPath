void _m512i_vec_dump( __m512i vec ){
  int vec_dump[16] __attribute__((aligned(64)));
  //*((__m512i *) vec_dump) = vec;
  _mm512_store_epi32(vec_dump, vec);
  printf( "0x%08x 0x%08x 0x%08x 0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n", vec_dump[0], vec_dump[1], vec_dump[2],
          vec_dump[3], vec_dump[4], vec_dump[5], vec_dump[6], vec_dump[7] );
  printf( "0x%08x 0x%08x 0x%08x 0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n", vec_dump[8], vec_dump[9],
          vec_dump[10], vec_dump[11], vec_dump[12], vec_dump[13], vec_dump[14], vec_dump[15] );
}
