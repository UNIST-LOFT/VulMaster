static int jpc_siz_getparms(jpc_ms_t *ms, jpc_cstate_t *cstate,
  jas_stream_t *in)
{
	jpc_siz_t *siz = &ms->parms.siz;
	unsigned int i;
	uint_fast8_t tmp;

	/* Eliminate compiler warning about unused variables. */
	cstate = 0;

	if (jpc_getuint16(in, &siz->caps) ||
	  jpc_getuint32(in, &siz->width) ||
	  jpc_getuint32(in, &siz->height) ||
	  jpc_getuint32(in, &siz->xoff) ||
	  jpc_getuint32(in, &siz->yoff) ||
	  jpc_getuint32(in, &siz->tilewidth) ||
	  jpc_getuint32(in, &siz->tileheight) ||
	  jpc_getuint32(in, &siz->tilexoff) ||
	  jpc_getuint32(in, &siz->tileyoff) ||
	  jpc_getuint16(in, &siz->numcomps)) {
		return -1;
	}
	if (!siz->width || !siz->height || !siz->tilewidth ||
	  !siz->tileheight || !siz->numcomps) {
		return -1;
	}
	if (!(siz->comps = jas_alloc2(siz->numcomps, sizeof(jpc_sizcomp_t)))) {
		return -1;
	}
	for (i = 0; i < siz->numcomps; ++i) {
		if (jpc_getuint8(in, &tmp) ||
		  jpc_getuint8(in, &siz->comps[i].hsamp) ||
		  jpc_getuint8(in, &siz->comps[i].vsamp)) {
			jas_free(siz->comps);
			return -1;
		}
		<vul-start>siz->comps[i].sgnd = (tmp >> 7) & 1;<vul-end>
		siz->comps[i].prec = (tmp & 0x7f) + 1;
	}
	if (jas_stream_eof(in)) {
		jas_free(siz->comps);
		return -1;
	}
	return 0;
}