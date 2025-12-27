<script lang="ts">

	import type { StepperConfig, StepperControlState, StepperDiag } from '$lib/types/models'
	import { postJsonRest, getJsonRest} from "$lib/stores/rest"
	import { onDestroy, onMount } from "svelte";
	import SettingsCard from './SettingsCard.svelte';
	import DcMotorControlForm from './DCMotorControlForm.svelte';
	import DcMotorSettingsForm from './DCMotorSettingsForm.svelte';

	export let restPath : string;
	export let name = "DC Motor";

</script>

<SettingsCard>
	<span slot="title">{name}</span>
	<DcMotorControlForm
		restPath={restPath + "/control"}
	>
		<DcMotorSettingsForm
			restPath={restPath + "/config"}
		></DcMotorSettingsForm>
	</DcMotorControlForm>
</SettingsCard>