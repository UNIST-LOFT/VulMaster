/*
 * This function reads the raster image data from the input TIFF for an image
 * tile and writes the data to the output PDF XObject image dictionary stream
 * for the tile.  It returns the amount written or zero on error.
 */

tsize_t t2p_readwrite_pdf_image_tile(T2P* t2p, TIFF* input, TIFF* output, ttile_t tile){

	uint16 edge=0;
	tsize_t written=0;
	unsigned char* buffer=NULL;
	tsize_t bufferoffset=0;
	unsigned char* samplebuffer=NULL;
	tsize_t samplebufferoffset=0;
	tsize_t read=0;
	uint16 i=0;
	ttile_t tilecount=0;
	/* tsize_t tilesize=0; */
	ttile_t septilecount=0;
	tsize_t septilesize=0;
#ifdef JPEG_SUPPORT
	unsigned char* jpt;
	float* xfloatp;
	uint32 xuint32=0;
#endif

	/* Fail if prior error (in particular, can't trust tiff_datasize) */
	if (t2p->t2p_error != T2P_ERR_OK)
		return(0);

	edge |= t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile);
	edge |= t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile);

	if( (t2p->pdf_transcode == T2P_TRANSCODE_RAW) && ((edge == 0)
#if defined(JPEG_SUPPORT) || defined(OJPEG_SUPPORT)
		|| (t2p->pdf_compression == T2P_COMPRESS_JPEG)
#endif
	)
	){
#ifdef CCITT_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_G4){
			buffer= (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			TIFFReadRawTile(input, tile, (tdata_t) buffer, t2p->tiff_datasize);
			if (t2p->tiff_fillorder==FILLORDER_LSB2MSB){
					TIFFReverseBits(buffer, t2p->tiff_datasize);
			}
			t2pWriteFile(output, (tdata_t) buffer, t2p->tiff_datasize);
			_TIFFfree(buffer);
			return(t2p->tiff_datasize);
		}
#endif
#ifdef ZIP_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_ZIP){
			buffer= (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			TIFFReadRawTile(input, tile, (tdata_t) buffer, t2p->tiff_datasize);
			if (t2p->tiff_fillorder==FILLORDER_LSB2MSB){
					TIFFReverseBits(buffer, t2p->tiff_datasize);
			}
			t2pWriteFile(output, (tdata_t) buffer, t2p->tiff_datasize);
			_TIFFfree(buffer);
			return(t2p->tiff_datasize);
		}
#endif
#ifdef OJPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_OJPEG){
			if(! t2p->pdf_ojpegdata){
				TIFFError(TIFF2PDF_MODULE, 
					"No support for OJPEG image %s with "
                                        "bad tables", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			buffer=(unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			_TIFFmemcpy(buffer, t2p->pdf_ojpegdata, t2p->pdf_ojpegdatalength);
			if(edge!=0){
				if(t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile)){
					buffer[7]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength >> 8) & 0xff;
					buffer[8]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength ) & 0xff;
				}
				if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile)){
					buffer[9]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth >> 8) & 0xff;
					buffer[10]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth ) & 0xff;
				}
			}
			bufferoffset=t2p->pdf_ojpegdatalength;
			bufferoffset+=TIFFReadRawTile(input, 
					tile, 
					(tdata_t) &(((unsigned char*)buffer)[bufferoffset]), 
					-1);
			((unsigned char*)buffer)[bufferoffset++]=0xff;
			((unsigned char*)buffer)[bufferoffset++]=0xd9;
			t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
			_TIFFfree(buffer);
			return(bufferoffset);
		}
#endif
		if(t2p->tiff_compression == COMPRESSION_JPEG){
			unsigned char table_end[2];
			uint32 count = 0;
			buffer= (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate " TIFF_SIZE_FORMAT " bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
                                          (TIFF_SIZE_T) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			if(TIFFGetField(input, TIFFTAG_JPEGTABLES, &count, &jpt) != 0) {
				if (count >= 4) {
                                        int retTIFFReadRawTile;
                    /* Ignore EOI marker of JpegTables */
					<vul-start>_TIFFmemcpy(buffer, jpt, count - 2);<vul-end>
					bufferoffset += count - 2;
                    /* Store last 2 bytes of the JpegTables */
					table_end[0] = buffer[bufferoffset-2];
					table_end[1] = buffer[bufferoffset-1];
					xuint32 = bufferoffset;
                                        bufferoffset -= 2;
                                        retTIFFReadRawTile= TIFFReadRawTile(
						input, 
						tile, 
						(tdata_t) &(((unsigned char*)buffer)[bufferoffset]), 
						-1);
                                        if( retTIFFReadRawTile < 0 )
                                        {
                                            _TIFFfree(buffer);
                                            t2p->t2p_error = T2P_ERR_ERROR;
                                            return(0);
                                        }
					bufferoffset += retTIFFReadRawTile;
                    /* Overwrite SOI marker of image scan with previously */
                    /* saved end of JpegTables */
					buffer[xuint32-2]=table_end[0];
					buffer[xuint32-1]=table_end[1];
				}
			}
			t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
			_TIFFfree(buffer);
			return(bufferoffset);
		}
		(void)0;
	}

	if(t2p->pdf_sample==T2P_SAMPLE_NOTHING){
		buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
		if(buffer==NULL){
			TIFFError(TIFF2PDF_MODULE, 
				"Can't allocate %lu bytes of memory for "
                                "t2p_readwrite_pdf_image_tile, %s", 
				(unsigned long) t2p->tiff_datasize, 
				TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}

		read = TIFFReadEncodedTile(
			input, 
			tile, 
			(tdata_t) &buffer[bufferoffset], 
			t2p->tiff_datasize);
		if(read==-1){
			TIFFError(TIFF2PDF_MODULE, 
				"Error on decoding tile %u of %s", 
				tile, 
				TIFFFileName(input));
			_TIFFfree(buffer);
			t2p->t2p_error=T2P_ERR_ERROR;
			return(0);
		}

	} else {

		if(t2p->pdf_sample == T2P_SAMPLE_PLANAR_SEPARATE_TO_CONTIG){
			septilesize=TIFFTileSize(input);
			septilecount=TIFFNumberOfTiles(input);
			/* tilesize=septilesize*t2p->tiff_samplesperpixel; */
			tilecount=septilecount/t2p->tiff_samplesperpixel;
			buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			samplebuffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(samplebuffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			samplebufferoffset=0;
			for(i=0;i<t2p->tiff_samplesperpixel;i++){
				read = 
					TIFFReadEncodedTile(input, 
						tile + i*tilecount, 
						(tdata_t) &(samplebuffer[samplebufferoffset]), 
						septilesize);
				if(read==-1){
					TIFFError(TIFF2PDF_MODULE, 
						"Error on decoding tile %u of %s", 
						tile + i*tilecount, 
						TIFFFileName(input));
						_TIFFfree(samplebuffer);
						_TIFFfree(buffer);
					t2p->t2p_error=T2P_ERR_ERROR;
					return(0);
				}
				samplebufferoffset+=read;
			}
			t2p_sample_planar_separate_to_contig(
				t2p,
				&(buffer[bufferoffset]),
				samplebuffer, 
				samplebufferoffset); 
			bufferoffset+=samplebufferoffset;
			_TIFFfree(samplebuffer);
		}

		if(buffer==NULL){
			buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			read = TIFFReadEncodedTile(
				input, 
				tile, 
				(tdata_t) &buffer[bufferoffset], 
				t2p->tiff_datasize);
			if(read==-1){
				TIFFError(TIFF2PDF_MODULE, 
					"Error on decoding tile %u of %s", 
					tile, 
					TIFFFileName(input));
				_TIFFfree(buffer);
				t2p->t2p_error=T2P_ERR_ERROR;
				return(0);
			}
		}

		if(t2p->pdf_sample & T2P_SAMPLE_RGBA_TO_RGB){
			t2p->tiff_datasize=t2p_sample_rgba_to_rgb(
				(tdata_t)buffer, 
				t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth
				*t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_RGBAA_TO_RGB){
			t2p->tiff_datasize=t2p_sample_rgbaa_to_rgb(
				(tdata_t)buffer, 
				t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth
				*t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_YCBCR_TO_RGB){
			TIFFError(TIFF2PDF_MODULE, 
				"No support for YCbCr to RGB in tile for %s", 
				TIFFFileName(input));
			_TIFFfree(buffer);
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_LAB_SIGNED_TO_UNSIGNED){
			t2p->tiff_datasize=t2p_sample_lab_signed_to_unsigned(
				(tdata_t)buffer, 
				t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth
				*t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}
	}

	if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile) != 0){
		t2p_tile_collapse_left(
			buffer, 
			TIFFTileRowSize(input),
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth,
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
	}


	t2p_disable(output);
	TIFFSetField(output, TIFFTAG_PHOTOMETRIC, t2p->tiff_photometric);
	TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, t2p->tiff_bitspersample);
	TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, t2p->tiff_samplesperpixel);
	if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile) == 0){
		TIFFSetField(
			output, 
			TIFFTAG_IMAGEWIDTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth);
	} else {
		TIFFSetField(
			output, 
			TIFFTAG_IMAGEWIDTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth);
	}
	if(t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile) == 0){
		TIFFSetField(
			output, 
			TIFFTAG_IMAGELENGTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		TIFFSetField(
			output, 
			TIFFTAG_ROWSPERSTRIP, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
	} else {
		TIFFSetField(
			output, 
			TIFFTAG_IMAGELENGTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength);
		TIFFSetField(
			output, 
			TIFFTAG_ROWSPERSTRIP, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength);
	}
	TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(output, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);

	switch(t2p->pdf_compression){
	case T2P_COMPRESS_NONE:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
		break;
#ifdef CCITT_SUPPORT
	case T2P_COMPRESS_G4:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
		break;
#endif
#ifdef JPEG_SUPPORT
	case T2P_COMPRESS_JPEG:
		if (t2p->tiff_photometric==PHOTOMETRIC_YCBCR) {
			uint16 hor = 0, ver = 0;
			if (TIFFGetField(input, TIFFTAG_YCBCRSUBSAMPLING, &hor, &ver)!=0) {
				if (hor != 0 && ver != 0) {
					TIFFSetField(output, TIFFTAG_YCBCRSUBSAMPLING, hor, ver);
				}
			}
			if(TIFFGetField(input, TIFFTAG_REFERENCEBLACKWHITE, &xfloatp)!=0){
				TIFFSetField(output, TIFFTAG_REFERENCEBLACKWHITE, xfloatp);
			}
		}
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
		TIFFSetField(output, TIFFTAG_JPEGTABLESMODE, 0); /* JPEGTABLESMODE_NONE */
		if(t2p->pdf_colorspace & (T2P_CS_RGB | T2P_CS_LAB)){
			TIFFSetField(output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
			if(t2p->tiff_photometric != PHOTOMETRIC_YCBCR){
				TIFFSetField(output, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
			} else {
				TIFFSetField(output, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RAW);
			}
		}
		if(t2p->pdf_colorspace & T2P_CS_GRAY){
			(void)0;
		}
		if(t2p->pdf_colorspace & T2P_CS_CMYK){
			(void)0;
		}
		if(t2p->pdf_defaultcompressionquality != 0){
			TIFFSetField(output, 
				TIFFTAG_JPEGQUALITY, 
				t2p->pdf_defaultcompressionquality);
		}
		break;
#endif
#ifdef ZIP_SUPPORT
	case T2P_COMPRESS_ZIP:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
		if(t2p->pdf_defaultcompressionquality%100 != 0){
			TIFFSetField(output, 
				TIFFTAG_PREDICTOR, 
				t2p->pdf_defaultcompressionquality % 100);
		}
		if(t2p->pdf_defaultcompressionquality/100 != 0){
			TIFFSetField(output, 
				TIFFTAG_ZIPQUALITY, 
				(t2p->pdf_defaultcompressionquality / 100));
		}
		break;
#endif
	default:
		break;
	}

	t2p_enable(output);
	t2p->outputwritten = 0;
	bufferoffset = TIFFWriteEncodedStrip(output, (tstrip_t) 0, buffer,
					     TIFFStripSize(output)); 
	if (buffer != NULL) {
		_TIFFfree(buffer);
		buffer = NULL;
	}
	if (bufferoffset == -1) {
		TIFFError(TIFF2PDF_MODULE, 
			  "Error writing encoded tile to output PDF %s", 
			  TIFFFileName(output));
		t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	
	written = t2p->outputwritten;
	
	return(written);
}