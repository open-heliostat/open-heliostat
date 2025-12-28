<script lang="ts">
	import SettingsCard from './SettingsCard.svelte';
	import DcMotorControlForm from './DCMotorControlForm.svelte';
	import DcMotorSettingsForm from './DCMotorSettingsForm.svelte';

	export let restPath : string;
	export let name = "DC Motor";

</script>

<SettingsCard>
	{#snippet title()}
		<span>{name}</span>
	{/snippet}
	<DcMotorControlForm
		restPath={restPath + "/control"}
	>
		<DcMotorSettingsForm
			restPath={restPath + "/config"}
		></DcMotorSettingsForm>
	</DcMotorControlForm>
</SettingsCard>