//*** ���������� ************************************************************

// �g�p����摜�E�t�H���g�̐錾
var FontS = new Font( "Impact", Vsd.Height / 30, FONT_FIXED );
var FontM = new Font( "Impact", Vsd.Height / 20, FONT_FIXED );
var FontM_Outline = new Font( "Impact", Vsd.Height / 20, FONT_OUTLINE | FONT_FIXED );

// ���[�^�[���E�ɕ\������ꍇ 1�ɐݒ�
var MeterRight = 1;

// ����T�C�Y�ɉ����ă��[�^�[�摜�����T�C�Y
var Scale = Vsd.Height / 720;

var MeterR = 150 * Scale;
var MeterX	= MeterRight ? Vsd.Width  - MeterR * 2: 0;
var MeterY	= Vsd.Height - MeterR * 2;
var MeterCx = MeterX + MeterR;
var MeterCy = MeterY + MeterR;

// �X�s�[�h�O���t�T�C�Y�v�Z
var SpdX1 = MeterRight ? 8 : 300 * Scale * 1.1;
var SpdX2 = MeterRight ? Vsd.Width - 300 * Scale * 1.1 : Vsd.Width - 8;
var SpdY1 = Vsd.Height - 300 * Scale;
var SpdY2 = Vsd.Height - 8;

// �X�s�[�h���[�^�p�ō����v�Z
var MaxSpeed = Math.ceil( Vsd.MaxSpeed / 10 ) * 10;

//*** ���[�^�[�`�揈�� ******************************************************

function Draw(){
	Vsd.DrawCircle( MeterCx, MeterCy, MeterR, 0x80404040, DRAW_FILL );
	
	// ���[�^�[�ڐ���`��
	Vsd.DrawMeterScale(
		MeterCx, MeterCy, MeterR,
		MeterR * 0.1,  2, 0xFFFFFF,
		MeterR * 0.05, 1, 0xFFFFFF,
		2, 135, 45,
		MeterR * 0.75,
		MaxSpeed, 12, 0xFFFFFF,
		FontM
	);
	
	Vsd.DrawGSnake(	MeterCx, MeterCy, MeterR / 1.5, 5, 2, 0x00FF00, 0x008000 );
	
	var Speed = ~~Vsd.Speed;
	if     ( Speed <  10 ) Speed = "  " + Speed;
	else if( Speed < 100 ) Speed = " "  + Speed;
	
	Vsd.DrawText(
		MeterCx - FontM.GetTextWidth( Speed ) / 2, MeterCy + MeterR / 2,
		Speed, FontM, 0xFFFFFF
	);
	
	// G ���l
	var Accel = Math.sqrt( Vsd.Gx * Vsd.Gx + Vsd.Gy * Vsd.Gy ).toFixed( 1 ) + "G";
	Vsd.DrawText(
		MeterCx - FontS.GetTextWidth( Accel ) / 2, MeterCy + MeterR / 2 - FontS.Height,
		Accel, FontS, 0xFFFFFF
	);
	
	// �X�s�[�h���[�^�[�j
	Vsd.DrawNeedle(
		MeterCx, MeterCy, MeterR * 0.95, 0,
		135, 45, Vsd.Speed / MaxSpeed, 0xFF0000, 3
	);
	
	// ���s�O��
	//Vsd.PutImage( 0, 0, ImgMapBG );
	Vsd.DrawMap(
		8 * Scale, 8 * Scale, 500 * Scale, 300 * Scale,
		ALIGN_TOP | ALIGN_LEFT,
		2, 5, 0xFF0000, 0xFFFF00, 0x00FF00, 0xFF0000
	);
	
	// ���b�v�^�C��
	Vsd.DrawLapTime( Vsd.Width - 1, 0, ALIGN_TOP | ALIGN_RIGHT, FontM_Outline );
	
	// �X�s�[�h�O���t
	Vsd.DrawGraph( SpdX1, SpdY1, SpdX2, SpdY2, FontM, GRAPH_SPEED | GRAPH_GX | GRAPH_GY | GRAPH_TILE );
}
