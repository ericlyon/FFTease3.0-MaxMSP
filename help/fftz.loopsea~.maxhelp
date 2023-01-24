{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 8,
			"minor" : 5,
			"revision" : 3,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 2397.0, 990.0, 657.0, 711.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-32",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 295.0, 556.0, 105.0, 20.0 ],
					"text" : "More details here:"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-30",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 5,
							"revision" : 3,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 1652.0, 542.0, 927.0, 679.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"assistshowspatchername" : 0,
						"boxes" : [ 							{
								"box" : 								{
									"id" : "obj-49",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 262.5, 502.0, 103.0, 22.0 ],
									"presentation_linecount" : 2,
									"text" : "randtransp 0.25 4"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-48",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 229.5, 299.0, 151.0, 20.0 ],
									"text" : "set the synthesis threshold"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-46",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 358.0, 71.0, 179.0, 20.0 ],
									"text" : "set the global transposition level"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-44",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 40.0, 299.0, 29.5, 22.0 ],
									"text" : "0."
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-42",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 40.0, 472.0, 420.0, 20.0 ],
									"text" : "randomly assign transposition levels between a minmum and maximum value"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-40",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 480.0, 407.0, 390.0, 20.0 ],
									"text" : "randomly choose transposition levels from a user-provided set of values"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-38",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 599.0, 553.0, 127.0, 22.0 ],
									"text" : "transp_choose 1 2 3 4"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-36",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 574.0, 522.0, 117.0, 22.0 ],
									"text" : "transp_choose 1 0.5"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-34",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 542.0, 493.0, 164.0, 22.0 ],
									"text" : "transp_choose 0.25 0.5 1 2 3"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-32",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 504.0, 462.0, 177.0, 22.0 ],
									"presentation_linecount" : 2,
									"text" : "transp_choose 1 1.25 1.5 1.875"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-31",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 480.0, 431.0, 157.0, 22.0 ],
									"text" : "transp_choose 0.6666 1 1.5"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-29",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 480.0, 606.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-28",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 161.0, 502.0, 96.0, 22.0 ],
									"text" : "randtransp 0.5 2"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-27",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 40.0, 502.0, 119.0, 22.0 ],
									"text" : "randtransp 0.95 1.05"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-25",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 40.0, 587.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-24",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 185.0, 299.0, 32.0, 22.0 ],
									"presentation_linecount" : 2,
									"text" : "0.25"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-23",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 139.0, 299.0, 29.5, 22.0 ],
									"text" : "0.1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-21",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 85.0, 299.0, 39.0, 22.0 ],
									"text" : "0.001"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-19",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 85.0, 353.0, 77.0, 22.0 ],
									"text" : "synthresh $1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-17",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 85.0, 388.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-16",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 270.0, 73.0, 70.0, 22.0 ],
									"text" : "loadmess 1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-13",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 328.0, 193.0, 78.0, 22.0 ],
									"text" : "transpose $1"
								}

							}
, 							{
								"box" : 								{
									"floatoutput" : 1,
									"id" : "obj-11",
									"maxclass" : "slider",
									"min" : 0.1,
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 328.0, 104.0, 225.0, 24.0 ],
									"size" : 1.9
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-10",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 328.0, 146.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-8",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 328.0, 232.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-7",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 120.0, 104.0, 121.0, 20.0 ],
									"text" : "toggle oscillator bank"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-4",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 92.0, 142.0, 70.0, 22.0 ],
									"text" : "oscbank $1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-2",
									"maxclass" : "toggle",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "int" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 92.0, 104.0, 24.0, 24.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-22",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 92.0, 177.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-13", 0 ],
									"source" : [ "obj-10", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-10", 0 ],
									"source" : [ "obj-11", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-8", 0 ],
									"source" : [ "obj-13", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-10", 0 ],
									"midpoints" : [ 279.5, 136.0, 337.5, 136.0 ],
									"source" : [ "obj-16", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-17", 0 ],
									"source" : [ "obj-19", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-4", 0 ],
									"source" : [ "obj-2", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-19", 0 ],
									"source" : [ "obj-21", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-19", 0 ],
									"midpoints" : [ 148.5, 336.5, 94.5, 336.5 ],
									"source" : [ "obj-23", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-19", 0 ],
									"midpoints" : [ 194.5, 336.5, 94.5, 336.5 ],
									"source" : [ "obj-24", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-25", 0 ],
									"source" : [ "obj-27", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-25", 0 ],
									"midpoints" : [ 170.5, 530.0, 49.5, 530.0 ],
									"source" : [ "obj-28", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 0 ],
									"source" : [ "obj-31", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 0 ],
									"source" : [ "obj-32", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 0 ],
									"source" : [ "obj-34", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 0 ],
									"source" : [ "obj-36", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 0 ],
									"source" : [ "obj-38", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-4", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-19", 0 ],
									"midpoints" : [ 49.5, 336.5, 94.5, 336.5 ],
									"source" : [ "obj-44", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-25", 0 ],
									"midpoints" : [ 272.0, 555.0, 49.5, 555.0 ],
									"source" : [ "obj-49", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 299.0, 612.0, 63.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"tags" : ""
					}
,
					"text" : "p oscbank"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-29",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 5,
							"revision" : 3,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 1338.0, 1276.0, 835.0, 545.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"assistshowspatchername" : 0,
						"boxes" : [ 							{
								"box" : 								{
									"id" : "obj-32",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 593.0, 215.0, 105.0, 22.0 ],
									"text" : "randspeed 0.1 10."
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-31",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 574.0, 186.0, 117.0, 22.0 ],
									"presentation_linecount" : 2,
									"text" : "randspeed -1 -0.955"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-30",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 561.0, 156.0, 85.0, 22.0 ],
									"text" : "randspeed 2 4"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-28",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 544.0, 129.0, 119.0, 22.0 ],
									"text" : "randspeed 0.99 1.01"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-27",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 523.0, 104.0, 125.0, 22.0 ],
									"text" : "randspeed 0.005 0.02"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-25",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 446.0, 70.0, 348.0, 20.0 ],
									"text" : "randomize the loop speeds: args are min and max speeds in Hz"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-22",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 523.0, 254.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-21",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 403.0, 281.0, 107.0, 22.0 ],
									"presentation_linecount" : 2,
									"text" : "setloops 250 5000"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-20",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 294.0, 281.0, 100.0, 22.0 ],
									"text" : "setloops 250 250"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-19",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 76.0, 251.0, 331.0, 20.0 ],
									"text" : "set the loops: args are min  and max duration in milliseconds"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-23",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 76.0, 281.0, 93.0, 22.0 ],
									"text" : "setloops 50 100"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-16",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 173.0, 281.0, 113.0, 22.0 ],
									"text" : "setloops 2000 2100"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-17",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 76.0, 358.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-15",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 263.0, 129.0, 155.0, 20.0 ],
									"text" : "reset all loops to their origin"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-13",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 263.0, 186.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-11",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 263.0, 156.0, 78.0, 22.0 ],
									"text" : "restart_loops"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-7",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 77.0, 91.0, 150.0, 20.0 ],
									"text" : "set the global speed"
								}

							}
, 							{
								"box" : 								{
									"format" : 6,
									"id" : "obj-5",
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "bang" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 77.0, 147.0, 50.0, 22.0 ]
								}

							}
, 							{
								"box" : 								{
									"floatoutput" : 1,
									"id" : "obj-3",
									"maxclass" : "slider",
									"min" : -2.0,
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"parameter_enable" : 0,
									"patching_rect" : [ 77.0, 118.0, 105.0, 20.0 ],
									"size" : 4.0
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-2",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 77.0, 180.0, 74.0, 22.0 ],
									"text" : "setspeed $1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-12",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 77.0, 213.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-13", 0 ],
									"source" : [ "obj-11", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-17", 0 ],
									"midpoints" : [ 182.5, 326.5, 85.5, 326.5 ],
									"source" : [ "obj-16", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-12", 0 ],
									"source" : [ "obj-2", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-17", 0 ],
									"midpoints" : [ 303.5, 336.5, 85.5, 336.5 ],
									"source" : [ "obj-20", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-17", 0 ],
									"midpoints" : [ 412.5, 343.0, 85.5, 343.0 ],
									"source" : [ "obj-21", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-17", 0 ],
									"source" : [ "obj-23", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-27", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-28", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 0 ],
									"source" : [ "obj-3", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-30", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-31", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-22", 0 ],
									"source" : [ "obj-32", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"source" : [ "obj-5", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 299.0, 582.0, 112.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"tags" : ""
					}
,
					"text" : "p speed-and-phase"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-24",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 5,
							"revision" : 3,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 1328.0, 922.0, 1130.0, 989.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"assistshowspatchername" : 0,
						"boxes" : [ 							{
								"box" : 								{
									"id" : "obj-10",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 121.381444454193115, 146.309277772903442, 187.0, 20.0 ],
									"text" : "click this message to restore data"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-8",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 121.381444454193115, 795.752538561820984, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-7",
									"linecount" : 3,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 492.783477544784546, 32.752577781677246, 326.288649916648865, 47.0 ],
									"text" : "send current loop data out the rightmost outlet of loopsea~. The resulting message can be used to reset the current state of all the loops."
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-5",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 423.0, 32.752577781677246, 61.0, 22.0 ],
									"text" : "printloops"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-12",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 423.0, 89.0, 93.0, 22.0 ],
									"text" : "s loopsea-msgs"
								}

							}
, 							{
								"box" : 								{
									"fontsize" : 10.0,
									"id" : "obj-3",
									"linecount" : 41,
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 121.381444454193115, 168.309277772903442, 993.0, 467.0 ],
									"text" : "readloops 3531 3624 1.5 1. 2339 2432 1.875 1. 889 982 1. 1. 2061 2154 1. 1. 587 680 1.25 1. 2208 2301 1.5 1. 1153 1246 1.5 1. 691 784 1.5 1. 144 237 1.25 1. 1183 1276 1.875 1. 342 435 1. 1. 2349 2442 1.25 1. 2563 2656 1.875 1. 802 895 1.25 1. 1811 1904 1.25 1. 1045 1138 1. 1. 370 463 1. 1. 2403 2496 1. 1. 1480 1573 1.5 1. 1989 2082 1.25 1. 1494 1587 1. 1. 1894 1987 1.25 1. 3557 3650 1.25 1. 3178 3271 1.875 1. 2654 2747 1.25 1. 2425 2518 1. 1. 3524 3617 1.25 1. 3151 3244 1.25 1. 1598 1691 1.875 1. 2248 2341 1.5 1. 2917 3010 1.5 1. 2462 2555 1.25 1. 3486 3579 1.5 1. 327 420 1. 1. 900 993 1.5 1. 2441 2534 1.875 1. 2130 2223 1.25 1. 3526 3619 1.25 1. 3038 3131 1.875 1. 540 633 1.25 1. 463 556 1. 1. 2924 3017 1. 1. 421 514 1.5 1. 464 557 1. 1. 3034 3127 1.5 1. 1975 2068 1.25 1. 530 623 1.875 1. 2604 2697 1.5 1. 180 273 1.25 1. 2226 2319 1.875 1. 3412 3505 1.25 1. 1181 1274 1. 1. 3231 3324 1.25 1. 1289 1382 1. 1. 3112 3205 1.875 1. 3190 3283 1.25 1. 481 574 1. 1. 2660 2753 1.875 1. 1972 2065 1. 1. 1555 1648 1.25 1. 532 625 1.25 1. 1499 1592 1.25 1. 1192 1285 1.25 1. 881 974 1. 1. 974 1067 1.5 1. 1591 1684 1.25 1. 95 188 1.25 1. 282 375 1.5 1. 332 425 1. 1. 1883 1976 1.875 1. 853 946 1.25 1. 2035 2128 1.875 1. 3064 3157 1.25 1. 3585 3678 1.875 1. 828 921 1. 1. 188 281 1.25 1. 1139 1232 1.5 1. 2458 2551 1.875 1. 3648 3741 1. 1. 2918 3011 1. 1. 2088 2181 1. 1. 71 164 1.5 1. 831 924 1.5 1. 1440 1533 1.25 1. 3177 3270 1.25 1. 3101 3194 1.25 1. 2887 2980 1.25 1. 1461 1554 1. 1. 206 299 1. 1. 1638 1731 1.875 1. 166 259 1.875 1. 1089 1182 1.25 1. 1335 1428 1. 1. 321 414 1.875 1. 1627 1720 1.875 1. 1349 1442 1.875 1. 2934 3027 1.5 1. 1474 1567 1.25 1. 2993 3086 1. 1. 2453 2546 1.5 1. 65 158 1.25 1. 3515 3608 1.875 1. 1082 1175 1.25 1. 1600 1693 1.875 1. 1850 1943 1.875 1. 1706 1799 1.875 1. 597 690 1.25 1. 1736 1829 1.25 1. 1117 1210 1.25 1. 614 707 1. 1. 374 467 1.875 1. 2137 2230 1.25 1. 228 321 1.875 1. 2515 2608 1.5 1. 1122 1215 1.5 1. 1665 1758 1.875 1. 984 1077 1. 1. 1237 1330 1.5 1. 1751 1844 1.5 1. 337 430 1. 1. 463 556 1.25 1. 778 871 1. 1. 1192 1285 1.875 1. 708 801 1.25 1. 893 986 1.5 1. 1434 1527 1.25 1. 1795 1888 1.5 1. 328 421 1. 1. 2529 2622 1.5 1. 1730 1823 1. 1. 175 268 1.5 1. 3555 3648 1. 1. 1094 1187 1.25 1. 3005 3098 1.25 1. 1857 1950 1. 1. 2531 2624 1.25 1. 484 577 1. 1. 2739 2832 1.5 1. 3201 3294 1.5 1. 3536 3629 1. 1. 1549 1642 1.875 1. 418 511 1.5 1. 2900 2993 1.25 1. 3447 3540 1.25 1. 2175 2268 1.25 1. 486 579 1.5 1. 3061 3154 1. 1. 3302 3395 1. 1. 1971 2064 1. 1. 1605 1698 1.5 1. 3308 3401 1.25 1. 3048 3141 1. 1. 3103 3196 1.5 1. 1946 2039 1.875 1. 3120 3213 1.25 1. 3119 3212 1.5 1. 3444 3537 1.25 1. 1998 2091 1.5 1. 1863 1956 1.875 1. 1024 1117 1.875 1. 2511 2604 1.25 1. 62 155 1. 1. 2730 2823 1.25 1. 381 474 1.875 1. 1845 1938 1.25 1. 1347 1440 1.25 1. 320 413 1.25 1. 1623 1716 1.5 1. 1008 1101 1. 1. 400 493 1.875 1. 878 971 1.5 1. 1269 1362 1.5 1. 2435 2528 1.5 1. 2461 2554 1.5 1. 1489 1582 1.5 1. 3137 3230 1. 1. 2733 2826 1. 1. 2975 3068 1.25 1. 825 918 1.875 1. 2451 2544 1. 1. 3223 3316 1. 1. 1049 1142 1.5 1. 1860 1953 1.5 1. 1176 1269 1.25 1. 1300 1393 1.875 1. 1631 1724 1.5 1. 231 324 1.875 1. 3412 3505 1.25 1. 3185 3278 1. 1. 1937 2030 1.5 1. 3453 3546 1.25 1. 2600 2693 1.5 1. 1523 1616 1.875 1. 887 980 1. 1. 900 993 1.5 1. 970 1063 1.25 1. 2552 2645 1. 1. 2617 2710 1.875 1. 3322 3415 1.5 1. 1532 1625 1.875 1. 1233 1326 1.5 1. 2569 2662 1.25 1. 1887 1980 1.25 1. 3560 3653 1.25 1. 284 377 1.5 1. 3446 3539 1. 1. 927 1020 1.5 1. 3491 3584 1.5 1. 3199 3292 1.25 1. 3290 3383 1.5 1. 2187 2280 1.5 1. 3085 3178 1. 1. 1793 1886 1.5 1. 2885 2978 1. 1. 1516 1609 1.25 1. 40 133 1. 1. 1689 1782 1. 1. 1498 1591 1.5 1. 3402 3495 1.875 1. 1698 1791 1.875 1. 128 221 1.5 1. 1947 2040 1.5 1. 1659 1752 1.25 1. 724 817 1.875 1. 2981 3074 1.5 1. 2014 2107 1.5 1. 2997 3090 1.25 1. 3490 3583 1.875 1. 2020 2113 1.875 1. 331 424 1.25 1. 2374 2467 1.5 1. 175 268 1. 1. 70 163 1.5 1. 2553 2646 1.5 1. 3435 3528 1.5 1. 3506 3599 1.875 1. 3434 3527 1.5 1. 2518 2611 1.25 1. 3618 3711 1. 1. 1458 1551 1. 1. 1114 1207 1.875 1. 3594 3687 1.5 1. 2027 2120 1.5 1. 3329 3422 1.25 1. 163 256 1.875 1. 551 644 1.5 1. 92 185 1.25 1. 760 853 1.25 1. 3368 3461 1.875 1. 1837 1930 1.875 1. 2528 2621 1. 1. 2111 2204 1.875 1. 1271 1364 1.875 1. 3103 3196 1.5 1. 1978 2071 1.875 1. 1172 1265 1.25 1. 3335 3428 1. 1. 3185 3278 1.5 1. 2763 2856 1.875 1. 2674 2767 1.5 1. 2410 2503 1.25 1. 3093 3186 1.25 1. 1656 1749 1.25 1. 170 263 1.875 1. 1999 2092 1.25 1. 1831 1924 1.875 1. 1571 1664 1.5 1. 3473 3566 1.5 1. 835 928 1.875 1. 1369 1462 1.25 1. 2568 2661 1.5 1. 1189 1282 1.875 1. 2354 2447 1. 1. 339 432 1.875 1. 3236 3329 1.875 1. 1642 1735 1.25 1. 2712 2805 1.25 1. 1445 1538 1. 1. 2855 2948 1.5 1. 2674 2767 1.875 1. 2907 3000 1.5 1. 1751 1844 1.875 1. 2744 2837 1.5 1. 158 251 1.875 1. 852 945 1.25 1. 2942 3035 1. 1. 2222 2315 1.5 1. 2503 2596 1.875 1. 1339 1432 1.5 1. 2930 3023 1.875 1. 1371 1464 1.25 1. 19 112 1. 1. 2288 2381 1. 1. 1103 1196 1.25 1. 3160 3253 1. 1. 703 796 1.25 1. 1312 1405 1.875 1. 257 350 1. 1. 2716 2809 1.25 1. 3502 3595 1.5 1. 672 765 1.25 1. 1697 1790 1. 1. 2987 3080 1. 1. 1698 1791 1. 1. 673 766 1. 1. 2566 2659 1.5 1. 2056 2149 1.5 1. 2090 2183 1.5 1. 1124 1217 1. 1. 2153 2246 1. 1. 1743 1836 1. 1. 728 821 1. 1. 297 390 1.875 1. 2895 2988 1.25 1. 1378 1471 1. 1. 609 702 1.5 1. 2547 2640 1.875 1. 808 901 1. 1. 102 195 1. 1. 1816 1909 1.875 1. 543 636 1. 1. 3016 3109 1.25 1. 3450 3543 1.25 1. 2574 2667 1.875 1. 186 279 1.5 1. 1490 1583 1.5 1. 2539 2632 1. 1. 1529 1622 1.5 1. 95 188 1. 1. 2465 2558 1.25 1. 2793 2886 1.25 1. 554 647 1.875 1. 1 94 1.25 1. 1817 1910 1.875 1. 3260 3353 1.25 1. 3306 3399 1.5 1. 1932 2025 1.25 1. 2855 2948 1.25 1. 1097 1190 1.25 1. 996 1089 1.5 1. 3504 3597 1.25 1. 2319 2412 1.5 1. 2 95 1.5 1. 3497 3590 1.875 1. 587 680 1. 1. 694 787 1.875 1. 3557 3650 1. 1. 235 328 1.25 1. 2813 2906 1.25 1. 206 299 1. 1. 609 702 1. 1. 3126 3219 1.25 1. 2821 2914 1. 1. 3650 3743 1.5 1. 3552 3645 1. 1. 188 281 1.875 1. 361 454 1.875 1. 2561 2654 1.25 1. 1679 1772 1.5 1. 792 885 1. 1. 2951 3044 1. 1. 2194 2287 1.25 1. 1947 2040 1. 1. 2082 2175 1. 1. 2975 3068 1.25 1. 211 304 1.875 1. 1053 1146 1.875 1. 1503 1596 1.5 1. 2465 2558 1.5 1. 3000 3093 1.875 1. 3422 3515 1.25 1. 2632 2725 1.5 1. 1804 1897 1.5 1. 1627 1720 1.5 1. 3324 3417 1.875 1. 3374 3467 1.5 1. 317 410 1.5 1. 3382 3475 1.875 1. 3082 3175 1. 1. 1107 1200 1.875 1. 1406 1499 1.5 1. 3088 3181 1.5 1. 3398 3491 1. 1. 67 160 1.875 1. 2207 2300 1.5 1. 1204 1297 1. 1. 687 780 1.5 1. 291 384 1.875 1. 2220 2313 1.25 1. 2581 2674 1.25 1. 828 921 1. 1. 3529 3622 1. 1. 2459 2552 1.875 1. 150 243 1.5 1. 517 610 1. 1. 339 432 1.25 1. 2138 2231 1.875 1. 2145 2238 1.875 1. 1335 1428 1.25 1. 1196 1289 1. 1. 2042 2135 1.5 1. 1292 1385 1.25 1. 2978 3071 1. 1. 3650 3743 1.5 1. 778 871 1. 1. 3108 3201 1. 1. 3433 3526 1.875 1. 770 863 1.875 1. 828 921 1.25 1. 3312 3405 1.5 1. 2445 2538 1.25 1. 845 938 1.5 1. 1293 1386 1. 1. 2518 2611 1. 1. 3301 3394 1.25 1. 2523 2616 1.25 1. 3109 3202 1.875 1. 1645 1738 1.5 1. 2304 2397 1.25 1. 1529 1622 1.875 1. 311 404 1. 1. 775 868 1. 1. 1999 2092 1. 1. 1115 1208 1.25 1. 2083 2176 1. 1. 842 935 1.875 1. 1090 1183 1.875 1. 2921 3014 1.25 1. 3161 3254 1.5 1. 1139 1232 1.875 1. 3615 3708 1.875 1. 3654 3747 1. 1. 1407 1500 1. 1. 478 571 1.5 1. 1162 1255 1.875 1. 3151 3244 1.5 1. 1136 1229 1.5 1. 707 800 1.5 1. 729 822 1.875 1. 574 667 1.5 1. 3043 3136 1.875 1. 713 806 1. 1. 11 104 1.875 1. 1982 2075 1. 1. 103 196 1.25 1. 2466 2559 1.875 1. 274 367 1.875 1. 155 248 1.5 1. 122 215 1.25 1. 865 958 1.5 1. 1893 1986 1.5 1. 3144 3237 1. 1. 836 929 1.875 1. 3335 3428 1.25 1. 2733 2826 1.5 1. 882 975 1.875 1. 3601 3694 1.5 1. 2813 2906 1.25 1. 23 116 1.875 1. 2315 2408 1.875 1. 835 928 1. 1. 1241 1334 1.5 1. 1184 1277 1. 1. 2384 2477 1. 1. 1430 1523 1.25 1. 2988 3081 1.5 1. 2329 2422 1.25 1. 1180 1273 1.25 1. 1158 1251 1.875 1. 3370 3463 1.5 1. 1990 2083 1.25 1. 983 1076 1.5 1. 1470 1563 1.25 1. 177 270 1.5 1. 1859 1952 1.25 1. 1891 1984 1. 1. 246 339 1. 1. 2682 2775 1. 1. 3607 3700 1. 1. 1879 1972 1. 1. 2369 2462 1. 1. 1394 1487 1.25 1. 2707 2800 1.25 1. 221 314 1. 1. 1822 1915 1.5 1. 2077 2170 1.875 1. 1387 1480 1.5 1. 393 486 1.25 1. 2079 2172 1.5 1. 190 283 1.875 1. 3048 3141 1.5 1. 3357 3450 1.5 1. 1060 1153 1.875 1. 2984 3077 1.5 1. 1375 1468 1.875 1. 3175 3268 1.5 1. 421 514 1.875 1. 3505 3598 1. 1. 1200 1293 1.25 1. 3314 3407 1.875 1. 1137 1230 1. 1. 2944 3037 1.875 1. 2487 2580 1.875 1. 2090 2183 1. 1. 410 503 1. 1. 1123 1216 1. 1. 2610 2703 1.25 1. 2621 2714 1. 1. 1459 1552 1.5 1. 722 815 1.5 1. 1822 1915 1.25 1. 778 871 1. 1."
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-1",
									"index" : 1,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "list" ],
									"patching_rect" : [ 121.381444454193115, 77.123711109161377, 30.0, 30.0 ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-3", 1 ],
									"midpoints" : [ 130.881444454193115, 137.21649444103241, 1104.881444454193115, 137.21649444103241 ],
									"source" : [ "obj-1", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-8", 0 ],
									"source" : [ "obj-3", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-12", 0 ],
									"source" : [ "obj-5", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 299.0, 482.0, 85.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"tags" : ""
					}
,
					"text" : "p loop-storage"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 13.0,
					"id" : "obj-66",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 105.0, 364.0, 65.0 ],
					"text" : "loopsea~ stores a spectral recording of a sound, and then creates loops on a per-bin basis, with each loop having its own length, speed of playback, and transposition factor (when in oscillator bank mode). ",
					"textcolor" : [ 0.498039, 0.498039, 0.498039, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 13.0,
					"id" : "obj-61",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 84.0, 190.0, 21.0 ],
					"text" : "FFT bin level algorithmic looper"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 48.0,
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 24.0, 281.0, 60.0 ],
					"text" : "fftz.loopsea~"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-22",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 222.0, 482.0, 71.0, 20.0 ],
					"text" : "record sync"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-20",
					"maxclass" : "ezdac~",
					"numinlets" : 2,
					"numoutlets" : 0,
					"patching_rect" : [ 74.0, 635.5, 45.0, 45.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-19",
					"maxclass" : "number~",
					"mode" : 2,
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "signal", "float" ],
					"patching_rect" : [ 162.0, 482.0, 56.0, 22.0 ],
					"sig" : 0.0
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-18",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 152.0, 224.0, 263.0, 20.0 ],
					"text" : "2. record sound into fftz.loopsea~ internal buffer"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-15",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 156.5, 658.5, 73.0, 22.0 ],
					"saved_object_attributes" : 					{
						"client_rect" : [ 100, 100, 500, 600 ],
						"parameter_enable" : 0,
						"parameter_mappable" : 0,
						"storage_rect" : [ 200, 200, 800, 500 ]
					}
,
					"text" : "pattrstorage",
					"varname" : "u018001268"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-13",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 156.5, 630.0, 56.0, 22.0 ],
					"restore" : 					{
						"live.gain~" : [ -20.945531293124446 ]
					}
,
					"text" : "autopattr",
					"varname" : "u409001205"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-7",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 350.0, 283.0, 255.0, 20.0 ],
					"text" : "1. load a sound that's at least 10 seconds long"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-11",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 112.0, 383.0, 91.0, 22.0 ],
					"text" : "r loopsea-msgs"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-9",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "bang", "" ],
					"patching_rect" : [ 74.0, 274.0, 34.0, 22.0 ],
					"text" : "sel 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 123.0, 222.0, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 309.0, 283.0, 35.0, 22.0 ],
					"text" : "open"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-4",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "signal", "bang" ],
					"patching_rect" : [ 309.0, 330.0, 47.0, 22.0 ],
					"text" : "sfplay~"
				}

			}
, 			{
				"box" : 				{
					"channels" : 1,
					"id" : "obj-3",
					"lastchannelcount" : 0,
					"maxclass" : "live.gain~",
					"numinlets" : 1,
					"numoutlets" : 4,
					"outlettype" : [ "signal", "", "float", "list" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 74.0, 468.0, 69.0, 136.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_longname" : "live.gain~",
							"parameter_mmax" : 6.0,
							"parameter_mmin" : -70.0,
							"parameter_shortname" : "Output Gain",
							"parameter_type" : 0,
							"parameter_unitstyle" : 4
						}

					}
,
					"varname" : "live.gain~"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "signal", "signal", "list" ],
					"patching_rect" : [ 74.0, 421.0, 113.0, 22.0 ],
					"text" : "fftz.loopsea~ 10000"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-2",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 74.0, 324.0, 93.0, 22.0 ],
					"text" : "acquire_sample"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-19", 0 ],
					"midpoints" : [ 130.5, 457.0, 171.5, 457.0 ],
					"source" : [ "obj-1", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-24", 0 ],
					"midpoints" : [ 177.5, 457.0, 308.5, 457.0 ],
					"source" : [ "obj-1", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-3", 0 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"midpoints" : [ 121.5, 412.0, 83.5, 412.0 ],
					"source" : [ "obj-11", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 1 ],
					"midpoints" : [ 83.5, 619.25, 109.5, 619.25 ],
					"order" : 0,
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 0 ],
					"order" : 1,
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"midpoints" : [ 318.5, 375.5, 83.5, 375.5 ],
					"source" : [ "obj-4", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"midpoints" : [ 132.5, 313.0, 318.5, 313.0 ],
					"order" : 0,
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-9", 0 ],
					"midpoints" : [ 132.5, 262.5, 83.5, 262.5 ],
					"order" : 1,
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-9", 0 ]
				}

			}
 ],
		"parameters" : 		{
			"obj-3" : [ "live.gain~", "Output Gain", 0 ],
			"parameterbanks" : 			{
				"0" : 				{
					"index" : 0,
					"name" : "",
					"parameters" : [ "-", "-", "-", "-", "-", "-", "-", "-" ]
				}

			}
,
			"inherited_shortname" : 1
		}
,
		"dependency_cache" : [ 			{
				"name" : "fftz.loopsea~.mxo",
				"type" : "iLaX"
			}
 ],
		"autosave" : 0
	}

}
