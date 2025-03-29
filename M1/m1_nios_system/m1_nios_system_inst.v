	m1_nios_system u0 (
		.clk_clk        (<connected-to-clk_clk>),        //     clk.clk
		.reset_reset_n  (<connected-to-reset_reset_n>),  //   reset.reset_n
		.sdram_addr     (<connected-to-sdram_addr>),     //   sdram.addr
		.sdram_ba       (<connected-to-sdram_ba>),       //        .ba
		.sdram_cas_n    (<connected-to-sdram_cas_n>),    //        .cas_n
		.sdram_cke      (<connected-to-sdram_cke>),      //        .cke
		.sdram_cs_n     (<connected-to-sdram_cs_n>),     //        .cs_n
		.sdram_dq       (<connected-to-sdram_dq>),       //        .dq
		.sdram_dqm      (<connected-to-sdram_dqm>),      //        .dqm
		.sdram_ras_n    (<connected-to-sdram_ras_n>),    //        .ras_n
		.sdram_we_n     (<connected-to-sdram_we_n>),     //        .we_n
		.ledr_export    (<connected-to-ledr_export>),    //    ledr.export
		.sw_export      (<connected-to-sw_export>),      //      sw.export
		.key_export     (<connected-to-key_export>),     //     key.export
		.hex_0_export   (<connected-to-hex_0_export>),   //   hex_0.export
		.hex_3_export   (<connected-to-hex_3_export>),   //   hex_3.export
		.address_export (<connected-to-address_export>), // address.export
		.data_export    (<connected-to-data_export>),    //    data.export
		.camera_export  (<connected-to-camera_export>)   //  camera.export
	);

