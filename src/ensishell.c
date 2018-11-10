/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "variante.h"
#include "readcmd.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
//

/*Structure dictionnaire. clef = pid*/

int pid_background = 0;

struct cellule *liste_pid_en_cours = NULL;

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>





int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	printf("Not implemented yet: can not execute %s\n", line);

	/* Remove this line when using parsecmd as it will free it */
	l = parsecmd(&line);
	execute_c(l);


	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}



/*le wait est necessaire car on connait pas l'ordre d'éxecution fils/pere
 * la console s'affiche avant l'exec du fils car le pere est rapide*/
int execute_commande(char **cmd, int back, char * in, char * out)
{
	pid_t pid;
	int status;
	//const char *quisuije = "le pere";
	pid = fork();
	if (pid == 0){
		//	/*redirige l'entrée pour la commande. rien apres exec
		/*redirige l'entrée pour la commande. rien apres exec*/
		if (in){
				int new_in = open(in, O_RDONLY); /*new_in est l'entrée correspondante a in dans la table des descript*/
				dup2(new_in, 0); /*0 est close d'office.*/ /*l'entrée stdin est maintenant le fichier associé a new_in*/
				/*apparait 2**/
				close(new_in);
		}
		if(out){
				int new_out = open(out, O_WRONLY);
				dup2(new_out,1);
				/*apparait 2*  */
				close(new_out);
		}

			execvp(cmd[0], cmd);
			printf("Unknown command\n");
			exit(0);
				//execute(cmd, in, out);
	}
	else if (pid == -1){
		perror("fork");
	}


	else
	{
		//si pas de tache en arriere, on attend que fils se termine
		if(!back){
			waitpid(pid,&status,0);
		}
		//le processus fils  s'execute en tache de fond
		else{
			inserer_tete(&liste_pid_en_cours, pid);

		}

	}
	return 0;
}

/*deux fork comme ca tout pipe peut s'éxecuter en arriere plan.
*/

int execute_pipe(char ***cmds, int back, char *in, char * out)
{
	pid_t pid;
	int status;
	//const char *quisuije = "le pere";
	pid = fork();
	if (pid == 0)
	{
		 char **cmd1 = cmds[0];
		 char **cmd2 = cmds[1];
		 int tuyau[2];

		 /*crée un tube.
		 contient deux descripteur de fichier sur tuyau:
		  -tuyau[0] qui est ouvert en mode lecture le fichier tuyau (pointe vers tuyau)
			-tuyau[1] qui est ouvert en mode ecriture sur le fichier tuyau
		 */
		 /*P|F <-> ls | grep u*/
		 pipe(tuyau);

		 pid_t second_pid;
		 //const char *quisuije = "le pere";
		 second_pid = fork();
		 /*Le fils lit dans le tuyau et écrit sur out(si il y a)/sorie standard */
		 if (second_pid == 0)
		 {
				 //  quisuije = "le fils";

					/*redirige l'entrée stdard(du processus fils  sur  le tuyau[0] = descripteur de fichier. 0 pour dire entrée  */
					/*maintenant a l'entree 0, il y a la struct pointer par le descrpt de fichier tuyau[0]. l'ancienne entrée standard a disparu.*/

				/*on suppr l'endroit d'ecriture sur le fichier pipe (fermer écoutilles tuyau en écriture)*/
				close(tuyau[1]);

				/*fils va lire sur tuyau*/
				dup2(tuyau[0], 0); /*tuyaux [0] devient entrée du processus*/
				/*la struct pointé par tuyau[0] appait 2* dans la table des descripteur(en 0 et a tuyau[0]). on veut juste stdin. il faut supprimer l'autre endroit vers lequel il pointe*/
				close(tuyau[0]); /*on vire l'ancien tuyau[0]*/

				if (out){
					/*fils va ecrire sur out*/
					int new_out =  open(out, O_WRONLY); /*new_out est l'entrée correspondante a out (path passé en param) dans la table des descript*/
					dup2(new_out,1); /*new_out devient sortie du processus*/
					close(new_out); /*on vire l'ancien new_out*/
				}

				//execute(cmd2, NULL, NULL);
				execvp(cmd2[0], cmd2);

				printf("Unknown command\n");
				exit(0);
		}
		else if (second_pid == -1)
		{
				perror("fork");
		}
		/*pour le pere on redirige la sortie de processus vers le tuyau*/
	  else
		{
				/*ferme écoutille du tuyau en lecture*/
				close(tuyau[0]);

				/*père va ecrire dans tuyau*/
				dup2(tuyau[1], 1);
				/*on suppr l'ancienne case*/
				close(tuyau[1]);

				if (in){
					/*père va lire dans in*/
					int new_in = open(in, O_RDONLY);
					dup2(new_in, 0);
					/*on suppr l'ancienne case*/
					close(new_in);
			  }
				//execute (cmd1, NULL, NULL);
				execvp(cmd1[0], cmd1);
		}
	}
	else if (pid == -1)
	{
				perror("fork");
	}
	else
	{
		//si pas de tache en arriere, on attend que fils se termine
		if(!back)
		{
			waitpid(pid,&status,0);
		}
		//le processus fils  s'execute en tache de fond
		else
		{
			inserer_tete(&liste_pid_en_cours, pid);
		}
	}
	return 0;
}

/*Le père a un seul fils à la fois.*/
/*On utilise un tuyau à chaque fois. */
/*Lecture(fd_in)--->[FILS]-->---TUYAU--->[PERE]--->fd_in = p[0] = sortie du tuyau_courant = entree du tuyau suivant.  */
/*c'est pas necessaire de fermer les src dupliqué dans le fils car il va mourir rapidement*/

int execute_multiple_pipe(char ***cmd, int back, char *in, char *out){

	/*entrée standard par defaut*/
	int fd_in = 0;
	/*si on specifie une entree standard*/
	if (in){
		 fd_in = open(in, O_RDONLY);
	}
	/*contient les descripteur de fichiers permettants au père/fils de comuniquer.*/
	int p[2];
	pid_t pidpipe;
	/*pid pour backgorund*/
	pid_t pid;
	/*background*/
	if ((pid = fork()) == -1){
		exit(EXIT_FAILURE);
	}
	else if(pid == 0){

	// tq il reste ue comande a éxecuter
	while (*cmd != NULL) {

		/*comunication père (et futur fils)*/
    pipe(p);
		//fprintf(stderr,"p[0]: %i\n,p[1]: %i\n", p[0], p[1]);
		/*on crée un fils(qui aura accès à ce tuyau) */
    if ((pidpipe = fork()) == -1)
      {
        exit(EXIT_FAILURE);
      }
		/*Si c'est le fils: il va lire dans fd_in*/
    else if (pidpipe == 0)
      {
        dup2(fd_in, 0); /*lecture dans fd_in*/
				/*on ferme l'accès à la lecture dans le tuyau. (Réservé pour le père.). */
				close(p[0]);

				/* si il y a une comande après, on écrit dans le tuyau comun au père.*/

				if (*(cmd + 1) != NULL){
          dup2(p[1], 1);
				}
				/*Sinon, ce fils est le dernier de la chaine, il écrira sur la sortie std (ou out si redirection) */
				else{
					if (out){
						int new_out = open(out, O_WRONLY);
						dup2(new_out, 1);
					}
				}

        execvp((*cmd)[0], *cmd);
        exit(EXIT_FAILURE);
      }

    else
      {
				/*on attend que le fils ai écrit dans tuyau.*/
        wait(NULL);
				/*on ferme l'écriture. Le père doit lire dans le tuyau.*/
        close(p[1]);
				/*p[0] est le descripteur de fichier qui pointe sur la sortie du tuyau. On le svgarde. le prochain fils ira lire dessus.*/
        fd_in = p[0];
        cmd++;
      }
		}
	}
	else{
		if (!back){
			wait(NULL);
		}
		else{
			inserer_tete(&liste_pid_en_cours, pid);
		}
	}

	return 0;
}





/*int execute_pipev2(char ***cmds, int back, int num)
{
	pid_t pid;
	int status;
	//const char *quisuije = "le pere";
	pid = fork();
	if (pid == 0)
	{

		 int *tuyaus[num - 1];
		 for(int i = 0; i<= num - 1; i++)
		 {
			 int tuyau[2];
			 pipe(tuyau);
			 tuyaus[i] = tuyau;
		 }

		 for(int i = 0; i< num - 1; i++)
		 {
			 pid_t second_pid = fork();
			 if(second_pid == 0)
			 {
				  dup2(tuyaus[i][0], 0);
	 			  close(tuyaus[i][0]);
	 			  close(tuyaus[i][1]);
					perror("pouet");
	 			  execvp(cmds[i+1][0], cmds[i+1]);

	 			  printf("Unknown command\n");
	 			  exit(0);
			 }
			 else if (second_pid == -1)
	 		 {
	 				perror("fork");
	 		 }
			 else
			 {
				 dup2(tuyaus[i][1], 1);
 				 close(tuyaus[i][0]);
 				 close(tuyaus[i][1]);
				 perror("salut");
 				 execvp(cmds[i][0], cmds[i]);
			 }
			 else if (second_pid == -1)
	 		 {
	 				perror("fork");
	 		 }
		 }
	}
	else if (pid == -1)
	{
				perror("fork");
	}
	else
	{
		//si pas de tache en arriere, on attend que fils se termine
		if(!back)
		{
			waitpid(pid,&status,0);
		}
		//le processus fils  s'execute en tache de fond
		else
		{
			inserer_tete(&liste_pid_en_cours, pid);
		}
	}
	return 0;
}*/


















/*
		Affiche sur la sortie standard les valeurs des cellules de la liste
		pointée par l.
*/
 void afficher(struct cellule *l)
{
		/* A implémenter! */
		while (l != NULL) {
				printf("%i -> ", l->val);
				l = l->suiv;
		}
		printf("FIN\n");
}

void inserer_tete(struct cellule **pl, int v)
{
		/*
				On alloue une nouvelle cellule, qui sera la tête de la nouvelle liste.
				L'opérateur sizeof calcule le nombre d'octet à allouer pour le type
				voulu.
		*/
		struct cellule *liste = malloc(sizeof(struct cellule));
		/*Programmation défensive : on vérification que malloc s'est bien passé*/
		assert(liste != NULL);
		/* On fixe les valeurs de cette cellule. */
		liste->val = v;
		liste->suiv = *pl;

		/* On fait pointer l'argument pl vers la nouvelle liste */
		*pl = liste;
}

/*
		Supprime de la liste pointée par pl la première occurrence de cellule
		contenant la valeur v.
*/
void supprimer_premiere_occurrence(struct cellule **pl, int v)
{
		/* A implémenter! */
		/*
				Cette fonction prend en argument un pointeur sur la liste, car cette
				dernière change lorsqu'on supprime la première cellule.
		*/
		struct cellule sent = { -1, *pl };
		struct cellule *p = &sent;
		/*
				En C, les conditions sont évaluées séquentiellement. L'expression à
				droite d'une condition logique && n'est évaluée que si l'expression à
				gauche est vraie.
		*/
		while (p->suiv != NULL && p->suiv->val != v) {
				p = p->suiv;
		}

		/* Cas occurence trouvée */
		if (p->suiv != NULL) {
				/*
						On rechaine les 2 cellules de la liste entourant l'occurrence et on
						libère la cellule trouvée.
				*/
				struct cellule *style = p->suiv;
				p->suiv = style->suiv;
				free(style);
		}
		*pl = sent.suiv;
}

/*on parcours les proccessus en cours.
si ils sont toujours là on affiche. sinon on supprime des processus en cours.
a essayer avec sleep + ps*/

void execute_job(){

	struct cellule *parcours_liste = liste_pid_en_cours;
	while (parcours_liste != NULL){
		int pid_cellule = (parcours_liste)->val;
		int resultat = waitpid(pid_cellule, NULL, WNOHANG);
		/*toujours là*/
		switch (resultat){

			case -1:
				perror("waitpid");
				break;

			case 0:
				printf("en cours: %i \n",pid_cellule);
				break;

			default:
				//printf("le processus a change\n %i",resultat);
				supprimer_premiere_occurrence(&liste_pid_en_cours,pid_cellule);
		}
		parcours_liste = parcours_liste->suiv;
	}
}
















int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line=0;
		//int i; //j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);

		/* If input stream closed, normal termination */
		if (!l) {

			terminate(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		execute_c(l);
 }

}


void execute_c (struct cmdline *l){


			/*l-> out pour toute la comd (pour tout le pipe)*/
			if (l->in) printf("in: %s\n", l->in); /*fichier ou lire*/
			if (l->out) printf("out: %s\n", l->out); /*fichier ou ecrire*/
			if (l->bg) printf("background (&)\n");

			int numcommands = 0;

			for (int i=0; l->seq[i]!=0; i++)
			{
				for (int j=0; l->seq[i][j] != 0; j++){
					printf("seq[%i][%i] = %s\n",i,j,l->seq[i][j]);
				}
				numcommands += 1;
			}

			//for (i=0; l->seq[i]!=0; i++) {
			/* Display each command of the pipe */
				//contient la commande et ses arguments ([ls, -a] ici seq[0] est le tableau correspondant a la commande avec arg

			//printf("constante: %i\n",O_RDWR);
			if(numcommands == 1)
			{
				char **cmd = l->seq[0];

				/*commande jobs*/
				if (strcmp(*cmd, "jobs")==0){
					execute_job();
				}
				/*sinon*/
				else{
	    		execute_commande(cmd, l->bg, l->in, l->out);
					//printf("%i\n", pid_background);
				}
			}
			else if(numcommands >= 2)
			{
				execute_multiple_pipe(l->seq, l->bg, l->in, l->out);
			}


		}
