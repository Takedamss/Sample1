#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/select.h>

struct sockaddr_in serv_addr;

/*サーバ側とソケットを通じて通信（チャット）をする関数
標準入力から文字を読み込みサーバ側へ一端送り、各クライアントへ送信される。
注意；ここでは、文字を送信した側のクライアントにも送信がいくため、2回表示されてしまうが、
これは、全てのクライアントに対してのサーバ側からの通信があったことを示している。また、Ctrl+cで
強制終了することもできる。また改善の余地も残っているため、文字を打っているうちに文字化けが出力
されることもある。その時はサーバ側とクライアント側両方を強制終了してから再度通信をすること。
さらに終了条件が「終了」と打つことだがまだ未完成であり、その後もチャットができてしまう(文字化けも含む)さらに、数字や、「あf」のように文字を打っている最中に送信した場合も文字化けが発生してしまう。
*/
int main(void){
	int error=0,c,sockfd,i;
	char buff[128];
	fd_set readfds;
	struct timeval tv;
	//ソケットを作る
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	//fd数字がわり当てられる作るごとに違う番号？
	if(sockfd < 0){
		perror("error:ソケットを作る");
		exit(1);
	}
	//サーバのアドレス作る
	memset(&serv_addr,0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("172.28.34.66");
	serv_addr.sin_port = htons(10000);//ポート番号
	//コネクションを張るための要求をサーバに送る
	error=connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in));
	if (error <0){
		perror("error:コネクション張るための要求サーバに送る");
		exit(1);
	}

	while(1){
		int r,buf_len,c;
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);	//監視対象
		FD_SET(0,&readfds);		//監視対象
		tv.tv_sec = 30;		//タイムアウト時間を30sec+100000usec に指定
		tv.tv_usec = 100000;
		r=select(sockfd+1,&readfds,NULL,NULL,&tv);//待ち
		if(r<0){
			perror("error:select");
			exit(1);
		}
		if(FD_ISSET(sockfd,&readfds) > 0){//サーバーから受け取り
			error=read(sockfd,buff,128);
			if (error == -1){
			perror("error:read");
			exit(1);
			}
			buff[error]='¥0';
			printf("%s¥n",buff);		//サーバーから受け取ったデータ表示
		}

		if(FD_ISSET(0,&readfds)> 0){//標準入力があった場合　入力
			buf_len = read(0,buff,128);//読み込む
			if (buf_len == -1){
				perror("error:read");
				exit(1);
			}
			buff[buf_len]='¥0';
			error=write(sockfd,buff,buf_len);//サーバ側に送信
			if (error == -1){
				perror("error:write");
				exit(1);
			}
			if(strcmp(buff,"終了")==0){
				finish(sockfd);
				if(close(sockfd) < 0){//ソケットを終了
					perror("error:close");
					exit(1);
				}
			}
			if(buf_len== -1){
				perror("error:read");
				exit(1);
			}
		}
		for(c=0;c<128;c++)buff[c]='¥0';	
	}
	return 0;
}

