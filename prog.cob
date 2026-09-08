                
        IDENTIFICATION DIVISION.
        CLASS-ID. MyBaseClass.
        FACTORY.
            PROCEDURE DIVISION.
            METHOD-ID. MyMethod IS FINAL.
                DATA DIVISION.
                LINKAGE SECTION.
                    01 VAR-IN  PIC 9(2).
                    01 VAR-OUT  PIC 9(2).
                PROCEDURE DIVISION USING VAR-IN 
                        RETURNING VAR-OUT.
                    MOVE VAR-IN TO VAR-OUT.
                    DISPLAY "Hello, world!".
            END METHOD MyMethod.
        END FACTORY.
        OBJECT.
            PROCEDURE DIVISION.
            METHOD-ID. SayHello.
                ENVIRONMENT DIVISION.
                CONFIGURATION SECTION.
                REPOSITORY.
                    CLASS MyBaseClass AS "MyClass".
                DATA DIVISION.
                WORKING-STORAGE SECTION.
                    01 an-object USAGE OBJECT REFERENCE MyBaseClass.
                    01 a-var PIC 9(2).

                PROCEDURE DIVISION.
                    DISPLAY an-object :: "MyMethod" ().
            END METHOD SayHello.
        END OBJECT.
        END CLASS MyBaseClass.

        PROGRAM-ID. prog.
        ENVIRONMENT DIVISION.
        CONFIGURATION SECTION.
        REPOSITORY.
            CLASS MyBaseClass AS "MyClass".
        DATA DIVISION.
        WORKING-STORAGE SECTION.
            01 an-object USAGE OBJECT REFERENCE MyBaseClass.
            01 a-var PIC 9(2).

        PROCEDURE DIVISION.
            INVOKE an-object "SayHello".
        END PROGRAM prog.