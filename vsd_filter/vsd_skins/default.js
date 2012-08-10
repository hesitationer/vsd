//*** ���������� ************************************************************

// �g�p����摜�E�t�H���g�̐錾
var ImgMeter = new Image( Vsd.SkinDir + "meter_bg.png" );
//var ImgMapBG = new Image( Vsd.SkinDir + "square.png" );
var ImgG     = new Image( ImgMeter );

var FontS = new Font( "Impact", Vsd.Height / 30 );
var FontM = new Font( "Impact", Vsd.Height / 23 );
var FontL = new Font( "Impact", Vsd.Height / 12 );
var FontM_Outline = new Font( "Impact", Vsd.Height / 23, FONT_FIXED | FONT_OUTLINE );

// ���[�^�[���E�ɕ\������ꍇ 1�ɐݒ�
var MeterLeft = 0;

// ����T�C�Y�ɉ����ă��[�^�[�摜�����T�C�Y
var Scale = Vsd.Height / 720;
if( Scale != 1 ){
	ImgMeter.Resize( ImgMeter.Width * Scale, ImgMeter.Height * Scale );
	ImgMapBG.Resize( ImgMapBG.Width * Scale, ImgMapBG.Height * Scale );
}

// ���W����\�ߌv�Z���Ă���
var MeterX	= MeterLeft ? Vsd.Width  - ImgMeter.Width : 0;
var MeterY	= Vsd.Height - ImgMeter.Height * 0.85
var MeterR  = 150 * Scale;
var MeterR2 = 126 * Scale;
var MeterCx = MeterX + MeterR;
var MeterCy = MeterY + MeterR;

// G �p�摜�����T�C�Y
ImgG.Resize( ImgG.Width * Scale / 2, ImgG.Height * Scale / 2 );

// G ���[�^�[�p�̍��W�v�Z
var MeterGX	 = MeterLeft ? Vsd.Width - ImgMeter.Width * 1.45 : ImgMeter.Width * 0.95;
var MeterGY	 = Vsd.Height - ImgG.Height;
var MeterGR  = MeterR / 2;
var MeterGR2 = MeterR2 / 2;
var MeterGCx = MeterGX + MeterGR;
var MeterGCy = MeterGY + MeterGR;

//*** ���[�^�[�`�揈�� ******************************************************

function Draw(){
	// ���[�^�[�摜�`��
	Vsd.PutImage( MeterX, MeterY, ImgMeter );
	
	// ���[�^�[�ڐ���`��
	Vsd.DrawMeterScale(
		MeterCx, MeterCy, MeterR2,
		MeterR2 * 0.1,  2, 0xFFFFFF,
		MeterR2 * 0.05, 1, 0xFFFFFF,
		5, 135, 45,
		MeterR2 * 0.75,
		Vsd.MaxSpeed, 12, 0xFFFFFF,
		FontM
	);
	
	// �X�s�[�h���l�\��
	var Speed = ~~Vsd.Speed;
	Vsd.DrawTextAlign(
		MeterCx, MeterCy + MeterR * 0.25, 
		ALIGN_HCENTER | ALIGN_VCENTER,
		Speed, FontL, 0xFFFFFF
	);
	
	Vsd.DrawTextAlign(
		MeterCx, MeterCy + MeterR * 0.5,
		ALIGN_HCENTER | ALIGN_VCENTER,
		"km/h", FontS, 0xFFFFFF
	);
	
	// �X�s�[�h���[�^�[�j
	Vsd.DrawNeedle(
		MeterCx, MeterCy, MeterR2 * 0.95, MeterR2 * -0.1,
		135, 45, Vsd.Speed / Vsd.MaxSpeed, 0xFF0000, 3
	);
	
	// G���[�^�[�p�l���摜�`��
	Vsd.PutImage( MeterGX, MeterGY, ImgG );
	Vsd.DrawLine( MeterGCx - MeterGR2, MeterGCy, MeterGCx + MeterGR2, MeterGCy, 0x802000 );
	Vsd.DrawLine( MeterGCx, MeterGCy - MeterGR2, MeterGCx, MeterGCy + MeterGR2, 0x802000 );
	Vsd.DrawCircle( MeterGCx, MeterGCy, MeterGR2 / 3,     0x802000 );
	Vsd.DrawCircle( MeterGCx, MeterGCy, MeterGR2 / 3 * 2, 0x802000 );
	
	// G ���l
	var Accel = Math.sqrt( Vsd.Gx * Vsd.Gx + Vsd.Gy * Vsd.Gy ).toFixed( 1 ) + "G";
	Vsd.DrawText(
		MeterGX, MeterGCy + MeterR / 2 - FontS.Height,
		Accel, FontS, 0xFFFFFF
	);
	
	// G �X�l�[�N
	Vsd.DrawGSnake(	MeterGCx, MeterGCy, MeterGR2 / 1.5, 5 * Scale, 2, 0xFF4000, 0x802000 );
	
	// ���s�O��
	//Vsd.PutImage( 0, 0, ImgMapBG );
	Vsd.DrawMap(
		8 * Scale, 8 * Scale, 500 * Scale, 300 * Scale,
		ALIGN_TOP | ALIGN_LEFT,
		2, 5, 0xFF0000, 0xFFFF00, 0x00FF00, 0xFF0000
	);
	
	// ���b�v�^�C��
	Vsd.DrawLapTime( Vsd.Width - 1, 0, ALIGN_TOP | ALIGN_RIGHT, FontM_Outline, 0xFFFFFF, 0, 0x00FFFF, 0xFF4000 );
	
	Vsd.DrawGraph( 0, 0, Vsd.Width, Vsd.Height, FontM );
}
