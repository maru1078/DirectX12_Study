float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	// Chapter4
	{
		// posはスクリーン座標？
		pos.x = pos.x / 960;
		pos.y = pos.y / 540;

		return float4(pos.xy, 1.0, 1.0);
	}
}