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
//#include<stdbool.h>
//要求受付数
#define MAXPENDING 5

struct sockaddr_in serv_addr;
 
struct client{
	int sockfd;	//ソケット接続用
};
/*2つの引数を比べてMAX値をとり返す関数
* 第一引数 nfds
* 第二引数 h	
*/
int max(int nfds,int h){
	int max=0;
	if(nfds<h)max=h;
	if(nfds>h)max=h;
	return max;
}
/*チャットサーバ側プログラム
  注意；サーバ側をCtrl+cで強制終了できるが、それを行った場合クライアント側にバグがおきるので、
  直ちにクライアント側もCtrl+cで終了すること。
  @author Masashi Takeda
  @return 正常であればint(1) エラーであればint(-1)とエラーの詳細を
*/
int main(void){
	int error,sockfd,h;
	char buff[128];
	char str[] ="退出しました";
	struct client cli[5];	//接続クライアント
	for(h=0;h<128;h++){
		buff[h]='¥0';
	}	

	//ソケットを作る
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	if(sockfd < 0){
		perror("error:sockfd");
		exit(1);
	}

	//アドレスを作る
	memset(&serv_addr,0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(10000);//ポート番号
	int flag=1;
	error=setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
	if(error <0){
		perror("error:setspckopt");
		exit(1);
	}

	//ソケットにアドレスを割り当てる
	error=bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in));
	if (error <0){
		perror("error:bind");
		exit(1);
	}

	//コネクション要求を待ち始めるよう指示
	error=listen(sockfd,MAXPENDING);
	if(error<0){
		perror("error:connect");
		exit(1);
	}
	int r,j,i,nfds=0,cli_number=0;		//ディスクリプタの最大値
	fd_set fds,readfds;		//接続待ち、受信待ち、送信待ちするディスクリプタの集合
	struct timeval tv;		//タイムアウト時間

	while(1){
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
	
		nfds=sockfd;
		for(j=0;j<cli_number;j++){
			FD_SET(cli[j].sockfd,&readfds);
			if(nfds<cli[j].sockfd)nfds=cli[j].sockfd;
		}

		//タイムアウト時間を30sec+500000usec に指定
		tv.tv_sec = 10;
		tv.tv_usec = 100000;

		r=select(nfds+1,&readfds,NULL,NULL,&tv);// rにはエラーの-1,タイムアウトの0,要求の総数1以上が来る
		if(r<0){
			perror("error:select");
			exit(1);
		}if(r> 0){
 
			if(FD_ISSET(sockfd,&readfds)>0 ){//接続要求がある時に反応

				if(cli_number>5){
					printf("上限に達したので接続不可");
					error=write(sockfd,buff,128);
					if(error <0){
						perror("error:write");
						exit(1);
					}
				}else {
					printf("クライアント[%d]の接続受付完了¥n",cli_number);//新規クライアント追加を通知
					cli[cli_number].sockfd= accept(sockfd,NULL,NULL);//accept
					if(cli[cli_number].sockfd <0){
						perror("error:accept");
						exit(1);
					}
					cli_number++;//クライアント数更新
				}	 
			}
			for(i=0;i<cli_number;i++){//全クライアントの要求を見る

				if(FD_ISSET(cli[i].sockfd,&readfds) ){	//cliからの通信
					
					//クライアントからデータ受け取る
					error=read(cli[i].sockfd,buff,128);
					if(error<0){
						perror("error:read");
						exit(1);
					}
					if(strcmp(buff,"終了")==0){
						printf("クライアント[%d]が退出しました",i);
						strcpy(buff,str);
						printf("%s",buff);
					}
					printf("受け取ったデータ %s¥n",buff);//受け取ったデータ表示

					//全クライアントにデータ送る
					for(j=0;j<cli_number;j++){
						error=write(cli[j].sockfd, buff, 128);
						if (error < 0){
							perror("error:write");
							exit(1);
						}
					}
				}
			}	
		}
	}
	//1秒待ってソケット終了する
	sleep(1);
	//クライアントと通信用ソケットを閉じる
	for(cli_number=0;cli_number<MAXPENDING;cli_number++){
		if(close(cli[cli_number].sockfd) <0){
			perror("error:close_newsock");
			exit(1);
		}
	}
	if(close(sockfd)){
		perror("error:close_sockfd");
		exit(1);
	}
	return 0;
}
